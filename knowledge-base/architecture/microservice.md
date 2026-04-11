# Go Microservice (Orchestrator)

> [!summary]
> The Go orchestrator is a microservice that manages task lifecycle, integrates with KeyDB/Redis queues, and invokes the C simulator as a subprocess to execute student firmware.

## Responsibility

The orchestrator is responsible for:
- **Task queue management** — pulling tasks from Redis, pushing results back
- **File mode operation** — loading tasks from JSON files for local testing
- **Simulator lifecycle** — spawning C simulator processes with timeouts
- **Output parsing** — extracting GPIO state, UART output, and statistics from simulator stdout
- **Result aggregation** — compiling structured results for return to the cloud lab

## Architecture

### Package Structure

```
orchestrator/
├── main.go                 # Entry point, CLI parsing, mode dispatching
├── go.mod                  # Go module definition
├── config.yaml             # Configuration file (optional)
├── types/
│   └── types.go            # Task and TaskResult struct definitions
├── keydb/
│   └── client.go           # Redis/KeyDB client wrapper (BLPOP, RPUSH)
└── simulator/
    └── runner.go           # Simulator process management, output parsing
```

### Two Operation Modes

| Mode | Purpose | Task Source | Result Destination |
|------|---------|-------------|-------------------|
| **queue** | Production (cloud lab) | KeyDB/Redis `simulator:tasks` | KeyDB/Redis `simulator:results` |
| **file** | Local testing | JSON file (`--file task.json`) | JSON file (`--output result.json`) |

## Data Flow

### Queue Mode

```
┌──────────────┐
│  KeyDB/Redis │
│ simulator:   │
│   tasks      │
└──────┬───────┘
       │ BLPOP (blocking)
       ▼
┌──────────────────┐
│  keydb.PopTask() │
└────────┬─────────┘
         │ Task struct
         ▼
┌─────────────────────────────────┐
│  simulator.RunTask(ctx, task)   │
│                                 │
│  1. Decode base64 → .bin file   │
│  2. Create temp file            │
│  3. exec.Command(simulator)     │
│  4. Wait with timeout           │
│  5. Parse stdout                │
└────────┬────────────────────────┘
         │ TaskResult struct
         ▼
┌──────────────────┐
│ keydb.PushResult │
└────────┬─────────┘
         │ RPUSH
         ▼
┌──────────────┐
│  KeyDB/Redis │
│ simulator:   │
│   results    │
└──────────────┘
```

### File Mode

```
┌─────────────────┐
│  task.json      │
│  (on disk)      │
└────────┬────────┘
         │ os.ReadFile
         ▼
┌──────────────────┐
│  json.Unmarshal  │
│  → Task struct   │
└────────┬─────────┘
         │
         ▼
┌─────────────────────────────────┐
│  simulator.RunTask(ctx, task)   │
└────────┬────────────────────────┘
         │ TaskResult struct
         ▼
┌──────────────────┐
│ json.Marshal     │
│ → result.json    │
└──────────────────┘
```

## Key Types

### Task (`types/types.go`)

```go
type Task struct {
    TaskID     string     `json:"task_id"`
    StudentID  string     `json:"student_id"`
    LabNumber  int        `json:"lab_number"`
    Binary     string     `json:"binary"`      // Base64-encoded .bin file
    TimeoutSec int        `json:"timeout_sec"`
    Config     TaskConfig `json:"config"`
}

type TaskConfig struct {
    GPIOInput string `json:"gpio_input,omitempty"`
    UARTInput string `json:"uart_input,omitempty"`
}
```

### TaskResult (`types/types.go`)

```go
type TaskResult struct {
    TaskID               string            `json:"task_id"`
    Status               string            `json:"status"` // "success", "error", "timeout"
    UARTOutput           string            `json:"uart_output,omitempty"`
    GPIOState            map[string]uint32 `json:"gpio_state,omitempty"`
    InstructionsExecuted int               `json:"instructions_executed,omitempty"`
    Cycles               int               `json:"cycles,omitempty"`
    ErrorMessage         string            `json:"error_message,omitempty"`
    Stdout               string            `json:"stdout,omitempty"`
}
```

## Simulator Runner (`runner.go`)

### Process Execution

```go
func (r *Runner) RunTask(ctx context.Context, task *types.Task) (*types.TaskResult, error) {
    // 1. Decode base64 firmware
    binaryData, _ := base64.StdEncoding.DecodeString(task.Binary)
    
    // 2. Create temp file
    tmpFile, _ := os.CreateTemp("", "firmware_*.bin")
    tmpFile.Write(binaryData)
    tmpFile.Close()
    
    // 3. Build command
    cmd := exec.CommandContext(ctx, r.simulatorPath, 
        "--max-steps", "10000", tmpFilename)
    
    // 4. Capture stdout
    var stdout strings.Builder
    cmd.Stdout = &stdout
    cmd.Stderr = &stdout
    
    // 5. Run with timeout
    if err := cmd.Start(); err != nil { ... }
    
    done := make(chan error, 1)
    go func() { done <- cmd.Wait() }()
    
    select {
    case <-time.After(execTimeout):
        cmd.Process.Kill()
        result.Status = "timeout"
    case err := <-done:
        r.parseOutput(result, stdout.String())
        result.Status = "success"
    }
    
    return result, nil
}
```

### Output Parsing

```go
func (r *Runner) parseOutput(result *types.TaskResult, output string) {
    statsRegex := regexp.MustCompile(`\[STATS\] Instructions executed: (\d+)`)
    gpioRegex := regexp.MustCompile(`\[GPIO\] PORT_(\w)_ODR = 0x([0-9A-Fa-f]+)`)
    uartRegex := regexp.MustCompile(`\[UART\] TX: 0x([0-9A-Fa-f]{2})`)
    regR0Regex := regexp.MustCompile(`R0:\s+0x([0-9A-Fa-f]+)`)
    
    for _, line := range strings.Split(output, "\n") {
        // Extract statistics
        if matches := statsRegex.FindStringSubmatch(line); matches != nil {
            result.InstructionsExecuted, _ = strconv.Atoi(matches[1])
        }
        
        // Extract GPIO state
        if matches := gpioRegex.FindStringSubmatch(line); matches != nil {
            portName := matches[1]
            val, _ := strconv.ParseUint(matches[2], 16, 32)
            result.GPIOState[fmt.Sprintf("PORT_%s_ODR", portName)] = uint32(val)
        }
        
        // Extract UART output
        if matches := uartRegex.FindStringSubmatch(line); matches != nil {
            val, _ := strconv.ParseUint(matches[1], 16, 16)
            result.UARTOutput += string(byte(val))
        }
        
        // Extract register values
        if matches := regR0Regex.FindStringSubmatch(line); matches != nil {
            val, _ := strconv.ParseUint(matches[1], 16, 32)
            result.GPIOState["R0"] = uint32(val)
        }
    }
}
```

## KeyDB Client (`keydb/client.go`)

```go
type Client struct {
    redis *redis.Client
    ctx   context.Context
}

func NewClient(cfg Config) *Client {
    rdb := redis.NewClient(&redis.Options{
        Addr:     cfg.Addr,
        Password: cfg.Password,
        DB:       0,
    })
    return &Client{redis: rdb, ctx: context.Background()}
}

func (c *Client) PopTask(queueName string, timeout time.Duration) (*types.Task, error) {
    result, err := c.redis.BLPop(c.ctx, timeout, queueName).Result()
    if err == redis.Nil {
        return nil, nil  // Queue empty
    }
    var task types.Task
    json.Unmarshal([]byte(result[1]), &task)
    return &task, nil
}

func (c *Client) PushResult(resultQueueName string, result *types.TaskResult) error {
    data, _ := json.Marshal(result)
    return c.redis.RPush(c.ctx, resultQueueName, data).Err()
}
```

## CLI Interface

```bash
# Queue mode (production)
./orchestrator --mode queue --redis localhost:6379

# File mode (testing)
./orchestrator --mode file \
    --file task.json \
    --output result.json \
    --simulator ../build/stm32_sim

# With custom timeout
./orchestrator --mode file --file task.json --timeout 10s
```

## Current Limitations

| Limitation | Impact |
|------------|--------|
| No OpenTelemetry integration | No metrics/tracing exported |
| Hardcoded step limit | Always passes `--max-steps 10000` regardless of task |
| No GPIO input injection | `TaskConfig.GPIOInput` is defined but not used |
| No UART input injection | `TaskConfig.UARTInput` is defined but not used |
| Regex-based parsing | Fragile to output format changes |
| No concurrent task processing | Single-threaded task processing (one at a time) |

## Interaction with Other Components

| Component | Interaction |
|-----------|-------------|
| [[cpu_core\|C Simulator]] | Spawns as subprocess, parses stdout |
| [[tech/keydb\|KeyDB/Redis]] | BLPOP tasks, RPUSH results |
| ITMO.clab | Receives tasks from web UI, returns results |

---

#go #microservice #orchestrator #keydb #redis #task-queue #subprocess
