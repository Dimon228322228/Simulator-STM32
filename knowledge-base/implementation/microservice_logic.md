# Microservice Logic

> [!summary]
> The Go orchestrator implements a task processing pipeline with two operational modes (queue and file), managing the lifecycle of C simulator processes and parsing their output for structured results.

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                   main.go                            │
│                                                      │
│  parseFlags() ──► Config                            │
│       │                                              │
│       ▼                                              │
│  keydb.NewClient() ──► keydb.Client                 │
│  simulator.NewRunner() ──► simulator.Runner         │
│       │                                              │
│       ├─► runQueueMode() ──► loop: PopTask → Run →  │
│       │                        PushResult            │
│       │                                              │
│       └─► runFileMode() ──► ReadFile → Run →        │
│                              WriteFile               │
└─────────────────────────────────────────────────────┘
```

## Entry Point (`main.go`)

### Flag Parsing

```go
func parseFlags() Config {
    config := Config{}
    
    flag.StringVar(&config.RedisAddr, "redis", "localhost:6379", "...")
    flag.StringVar(&config.RedisPassword, "redis-pass", "", "...")
    flag.StringVar(&config.TaskQueue, "task-queue", "simulator:tasks", "...")
    flag.StringVar(&config.ResultQueue, "result-queue", "simulator:results", "...")
    flag.StringVar(&config.SimulatorPath, "simulator", "./stm32_sim", "...")
    flag.DurationVar(&config.DefaultTimeout, "timeout", 5*time.Second, "...")
    flag.DurationVar(&config.PollInterval, "poll-interval", 1*time.Second, "...")
    flag.StringVar(&config.Mode, "mode", "queue", "queue or file")
    
    inputFile := flag.String("file", "", "Input task file")
    outputFile := flag.String("output", "", "Output result file")
    
    flag.Parse()
    
    // Store in environment for file mode access
    if *inputFile != "" {
        os.Setenv("INPUT_FILE", *inputFile)
    }
    if *outputFile != "" {
        os.Setenv("OUTPUT_FILE", *outputFile)
    }
    
    return config
}
```

### Main Function

```go
func main() {
    config := parseFlags()
    
    // Create context with signal handling
    ctx, cancel := context.WithCancel(context.Background())
    defer cancel()
    
    sigChan := make(chan os.Signal, 1)
    signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
    go func() {
        sig := <-sigChan
        log.Printf("[INFO] Received signal %v, shutting down...", sig)
        cancel()
    }()
    
    // Create clients
    keydbClient := keydb.NewClient(keydb.Config{
        Addr:     config.RedisAddr,
        Password: config.RedisPassword,
        DB:       0,
    })
    defer keydbClient.Close()
    
    simRunner := simulator.NewRunner(config.SimulatorPath, config.DefaultTimeout)
    
    // Dispatch mode
    if config.Mode == "queue" {
        runQueueMode(ctx, keydbClient, simRunner, config)
    } else if config.Mode == "file" {
        runFileMode(ctx, keydbClient, simRunner, config)
    }
}
```

## Queue Mode (`runQueueMode`)

```go
func runQueueMode(ctx context.Context, client *keydb.Client, 
                   runner *simulator.Runner, config Config) {
    for {
        select {
        case <-ctx.Done():
            return  // Graceful shutdown
        default:
        }
        
        // Blocking pop — waits until task available or timeout
        task, err := client.PopTask(config.TaskQueue, config.PollInterval)
        if err != nil {
            log.Printf("[ERROR] Failed to pop task: %v", err)
            time.Sleep(config.PollInterval)
            continue
        }
        
        if task == nil {
            continue  // Queue empty, poll again
        }
        
        log.Printf("[INFO] Received task %s (student: %s, lab: %d)",
            task.TaskID, task.StudentID, task.LabNumber)
        
        // Execute
        result, err := runner.RunTask(ctx, task)
        if err != nil {
            result = &types.TaskResult{
                TaskID:       task.TaskID,
                Status:       "error",
                ErrorMessage: err.Error(),
            }
        }
        
        // Push result back
        if err := client.PushResult(config.ResultQueue, result); err != nil {
            log.Printf("[ERROR] Failed to push result: %v", err)
        }
    }
}
```

### Queue Protocol

**Task message (LPUSH to `simulator:tasks`):**
```json
{
  "task_id": "task_001",
  "student_id": "student_42",
  "lab_number": 1,
  "binary": "BQUDGII=",
  "timeout_sec": 5,
  "config": {
    "gpio_input": "",
    "uart_input": ""
  }
}
```

**Result message (RPUSH to `simulator:results`):**
```json
{
  "task_id": "task_001",
  "status": "success",
  "uart_output": "",
  "gpio_state": {
    "R0": 5,
    "R1": 3,
    "R2": 8
  },
  "instructions_executed": 4,
  "cycles": 4,
  "error_message": "",
  "stdout": "[INIT] Loaded 8 bytes...\n..."
}
```

## File Mode (`runFileMode`)

```go
func runFileMode(ctx context.Context, client *keydb.Client, 
                 runner *simulator.Runner, config Config) {
    inputFile := os.Getenv("INPUT_FILE")
    outputFile := os.Getenv("OUTPUT_FILE")
    
    // Read task from JSON file
    data, _ := os.ReadFile(inputFile)
    var task types.Task
    json.Unmarshal(data, &task)
    
    // Load binary from separate file if not embedded
    if task.Binary == "" {
        binFile := os.Getenv("BINARY_FILE")
        if binFile != "" {
            binData, _ := os.ReadFile(binFile)
            task.Binary = base64.StdEncoding.EncodeToString(binData)
        }
    }
    
    // Execute
    result, err := runner.RunTask(ctx, &task)
    
    // Write result
    resultData, _ := json.MarshalIndent(result, "", "  ")
    os.WriteFile(outputFile, resultData, 0644)
}
```

## Simulator Runner (`runner.go`)

### Task Execution Pipeline

```go
func (r *Runner) RunTask(ctx context.Context, task *types.Task) (*types.TaskResult, error) {
    result := &types.TaskResult{
        TaskID:    task.TaskID,
        GPIOState: make(map[string]uint32),
    }
    
    // Step 1: Decode base64 binary
    binaryData, err := base64.StdEncoding.DecodeString(task.Binary)
    if err != nil {
        result.Status = "error"
        result.ErrorMessage = fmt.Sprintf("Failed to decode binary: %v", err)
        return result, nil
    }
    
    // Step 2: Create temp .bin file
    tmpFile, err := os.CreateTemp("", "firmware_*.bin")
    if err != nil {
        result.Status = "error"
        result.ErrorMessage = fmt.Sprintf("Failed to create temp file: %v", err)
        return result, nil
    }
    tmpFilename := tmpFile.Name()
    defer os.Remove(tmpFilename)  // Cleanup after execution
    
    tmpFile.Write(binaryData)
    tmpFile.Close()
    
    // Step 3: Determine timeout
    execTimeout := r.timeout
    if task.TimeoutSec > 0 {
        execTimeout = time.Duration(task.TimeoutSec) * time.Second
    }
    
    // Step 4: Build command
    cmd := exec.CommandContext(ctx, r.simulatorPath, 
        "--max-steps", "10000", tmpFilename)
    cmd.Dir = filepath.Dir(r.simulatorPath)
    
    var stdout strings.Builder
    cmd.Stdout = &stdout
    cmd.Stderr = &stdout
    
    // Step 5: Run with timeout
    startTime := time.Now()
    
    if err := cmd.Start(); err != nil {
        result.Status = "error"
        result.ErrorMessage = fmt.Sprintf("Failed to start simulator: %v", err)
        return result, nil
    }
    
    done := make(chan error, 1)
    go func() { done <- cmd.Wait() }()
    
    select {
    case <-time.After(execTimeout):
        cmd.Process.Kill()
        result.Status = "timeout"
        result.ErrorMessage = fmt.Sprintf("Execution time exceeded timeout (%v)", execTimeout)
        
    case err := <-done:
        duration := time.Since(startTime)
        result.Stdout = stdout.String()
        
        // Parse simulator output
        r.parseOutput(result, stdout.String())
        
        if result.Status == "" {
            result.Status = "success"
        }
        
        log.Printf("[INFO] Task %s completed in %v", task.TaskID, duration)
    }
    
    return result, nil
}
```

## Output Parsing (`parseOutput`)

```go
func (r *Runner) parseOutput(result *types.TaskResult, output string) {
    // Define regex patterns
    statsRegex := regexp.MustCompile(`\[STATS\] Instructions executed: (\d+)`)
    cyclesRegex := regexp.MustCompile(`\[STATS\] CPU cycles: (\d+)`)
    gpioRegex := regexp.MustCompile(`\[GPIO\] PORT_(\w)_ODR = 0x([0-9A-Fa-f]+)`)
    uartRegex := regexp.MustCompile(`\[UART\] TX: 0x([0-9A-Fa-f]{2})`)
    
    regR0Regex := regexp.MustCompile(`R0:\s+0x([0-9A-Fa-f]+)`)
    regR1Regex := regexp.MustCompile(`R1:\s+0x([0-9A-Fa-f]+)`)
    regR2Regex := regexp.MustCompile(`R2:\s+0x([0-9A-Fa-f]+)`)
    regR3Regex := regexp.MustCompile(`R3:\s+0x([0-9A-Fa-f]+)`)
    
    for _, line := range strings.Split(output, "\n") {
        // Extract stats
        if matches := statsRegex.FindStringSubmatch(line); matches != nil {
            result.InstructionsExecuted, _ = strconv.Atoi(matches[1])
        }
        
        // Extract GPIO
        if matches := gpioRegex.FindStringSubmatch(line); matches != nil {
            portName := matches[1]
            val, _ := strconv.ParseUint(matches[2], 16, 32)
            result.GPIOState[fmt.Sprintf("PORT_%s_ODR", portName)] = uint32(val)
        }
        
        // Extract UART
        if matches := uartRegex.FindStringSubmatch(line); matches != nil {
            val, _ := strconv.ParseUint(matches[1], 16, 16)
            result.UARTOutput += string(byte(val))
        }
        
        // Extract registers
        if matches := regR0Regex.FindStringSubmatch(line); matches != nil {
            val, _ := strconv.ParseUint(matches[1], 16, 32)
            result.GPIOState["R0"] = uint32(val)
        }
        // ... same for R1, R2, R3
    }
    
    // Check for errors
    if strings.Contains(output, "[ERROR]") {
        result.Status = "error"
        errorRegex := regexp.MustCompile(`\[ERROR\] (.+)`)
        if matches := errorRegex.FindStringSubmatch(output); matches != nil {
            result.ErrorMessage = matches[1]
        }
    }
}
```

## Error Handling Strategy

| Error Type | Handling | Result Status |
|------------|----------|---------------|
| Base64 decode failure | Return immediately | "error" |
| Temp file creation failure | Return immediately | "error" |
| Simulator start failure | Return immediately | "error" |
| Timeout | Kill process | "timeout" |
| Simulator exit error | Continue, parse output | "success" (if output valid) |
| Output parsing failure | Partial results | "success" (with empty fields) |

## Concurrency Model

**Current:** Single-threaded, sequential processing
- One task at a time
- Blocking BLPOP on queue
- No parallel execution

**Future enhancement:** Worker pool
```go
// Conceptual — not implemented
type WorkerPool struct {
    workers int
    tasks   chan *types.Task
    results chan *types.TaskResult
}

func (wp *WorkerPool) Start() {
    for i := 0; i < wp.workers; i++ {
        go func(id int) {
            for task := range wp.tasks {
                result := wp.runner.RunTask(ctx, task)
                wp.results <- result
            }
        }(i)
    }
}
```

## Configuration

### Default Values

| Parameter | Default | Description |
|-----------|---------|-------------|
| Redis address | `localhost:6379` | KeyDB/Redis server |
| Redis password | `""` | No authentication |
| Task queue | `simulator:tasks` | Queue name |
| Result queue | `simulator:results` | Queue name |
| Simulator path | `./stm32_sim` | Relative path |
| Timeout | 5 seconds | Per-task execution limit |
| Poll interval | 1 second | Queue polling frequency |
| Mode | `queue` | Default operation mode |

---

#go #microservice #orchestrator #task-pipeline #regex-parsing #timeout-handling #queue-protocol
