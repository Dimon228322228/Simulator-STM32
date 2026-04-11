# Go Microservice Patterns

> [!summary]
> The orchestrator implements common Go microservice patterns including queue-based processing, process management, and structured output parsing.

## Service Architecture

The orchestrator follows a **layered architecture**:

```
┌──────────────────────────────────────┐
│           main.go                     │
│      (Entry Point & Routing)          │
└──────────┬───────────────────────────┘
           │
    ┌──────┴──────┐
    │             │
┌───┴────┐   ┌───┴────────┐
│ keydb/  │   │ simulator/ │
│ client  │   │ runner     │
└────────┘   └────────────┘
     │              │
     │              │
┌────┴────┐   ┌────┴──────┐
│ types/   │   │ os/exec   │
│ Task,    │   │ Process   │
│ Result   │   │ Management│
└─────────┘   └───────────┘
```

## Key Patterns

### 1. Queue-Based Processing

```go
func runQueueMode(ctx context.Context, client *keydb.Client, runner *simulator.Runner) {
    for {
        select {
        case <-ctx.Done():
            return  // Graceful shutdown
        default:
        }
        
        task, err := client.PopTask(queue, pollInterval)
        if err != nil { ... }
        if task == nil { continue }  // Queue empty
        
        result := runner.RunTask(ctx, task)
        client.PushResult(queue, result)
    }
}
```

**Benefits:**
- Decoupled producers and consumers
- Natural load leveling (queue absorbs spikes)
- Easy horizontal scaling (multiple consumers)

### 2. Context-Based Cancellation

```go
ctx, cancel := context.WithCancel(context.Background())
defer cancel()

// Pass context to all operations
task, err := client.PopTask(ctx, queue)
result, err := runner.RunTask(ctx, task)
```

**Benefits:**
- Unified shutdown mechanism
- Timeout propagation through call chain
- Goroutine leak prevention

### 3. Process Management with Timeout

```go
func (r *Runner) RunTask(ctx context.Context, task *Task) (*Result, error) {
    cmd := exec.CommandContext(ctx, r.simPath, args...)
    
    done := make(chan error, 1)
    go func() { done <- cmd.Wait() }()
    
    select {
    case <-time.After(r.timeout):
        cmd.Process.Kill()
        return timeoutResult, nil
    case err := <-done:
        return parseResult(cmd.Stdout)
    }
}
```

**Benefits:**
- Guaranteed termination
- Resource cleanup on timeout
- No zombie processes

### 4. Regex-Based Output Parsing

```go
func (r *Runner) parseOutput(result *Result, output string) {
    patterns := map[string]*regexp.Regexp{
        "stats": regexp.MustCompile(`\[STATS\] Instructions executed: (\d+)`),
        "gpio":  regexp.MustCompile(`\[GPIO\] PORT_(\w)_ODR = 0x([0-9A-Fa-f]+)`),
        "uart":  regexp.MustCompile(`\[UART\] TX: 0x([0-9A-Fa-f]{2})`),
    }
    
    for _, line := range strings.Split(output, "\n") {
        for name, re := range patterns {
            if matches := re.FindStringSubmatch(line); matches != nil {
                // Extract and store
            }
        }
    }
}
```

**Trade-offs:**
- ✅ Simple, no changes to simulator required
- ⚠️ Fragile to output format changes
- ⚠️ No type safety (string parsing)

## Error Handling

### Graceful Degradation

```go
func (c *Client) PopTask(queue string, timeout time.Duration) (*Task, error) {
    result, err := c.redis.BLPop(c.ctx, timeout, queue).Result()
    if err == redis.Nil {
        return nil, nil  // Not an error — queue is just empty
    }
    if err != nil {
        return nil, fmt.Errorf("failed to pop task: %w", err)
    }
    // ...
}
```

### Error Wrapping

```go
return nil, fmt.Errorf("failed to decode binary: %w", err)
return nil, fmt.Errorf("failed to start simulator: %w", err)
```

**Benefits:**
- Preserves original error context
- Enables `errors.Is()` / `errors.As()` checks
- Clear error chain for debugging

## Configuration

### Flag-Based Configuration

```go
func parseFlags() Config {
    flag.StringVar(&config.RedisAddr, "redis", "localhost:6379", "...")
    flag.DurationVar(&config.DefaultTimeout, "timeout", 5*time.Second, "...")
    flag.Parse()
    return config
}
```

**Alternatives considered:**
- Environment variables (12-factor app)
- YAML/JSON config file
- Consul/etcd (service discovery)

**Decision:** CLI flags for simplicity and testability (easy to override in tests).

## Concurrency Model

### Current: Sequential Processing

```
Task 1 ──► Process ──► Result
              │
Task 2 ───────┘ (wait)
              │
Task 3 ───────┘ (wait)
```

### Future: Worker Pool

```go
type WorkerPool struct {
    workers int
    tasks   chan *Task
    results chan *Result
}

func (wp *WorkerPool) Start(ctx context.Context) {
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

## Testing Strategy

### Unit Tests (Go)

```go
func TestParseOutput(t *testing.T) {
    runner := NewRunner("./stm32_sim", 5*time.Second)
    result := &TaskResult{GPIOState: make(map[string]uint32)}
    
    output := `[STATS] Instructions executed: 4
[GPIO] PORT_A_ODR = 0x00000001
R0:  0x00000005`
    
    runner.parseOutput(result, output)
    
    assert.Equal(t, 4, result.InstructionsExecuted)
    assert.Equal(t, uint32(1), result.GPIOState["PORT_A_ODR"])
    assert.Equal(t, uint32(5), result.GPIOState["R0"])
}
```

### Integration Tests

```go
func TestFileMode(t *testing.T) {
    // Create temp task file
    taskFile := writeTaskJSON(t, testTask)
    outputFile := tempFile(t)
    
    // Run orchestrator
    cmd := exec.Command("./orchestrator",
        "--mode", "file",
        "--file", taskFile,
        "--output", outputFile,
        "--simulator", "../build/stm32_sim")
    
    err := cmd.Run()
    assert.NoError(t, err)
    
    // Verify result
    result := readResultJSON(t, outputFile)
    assert.Equal(t, "success", result.Status)
}
```

## Dependencies

| Module | Version | Purpose |
|--------|---------|---------|
| `github.com/go-redis/redis/v8` | v8.x | KeyDB/Redis client |
| `context` | stdlib | Cancellation, timeouts |
| `os/exec` | stdlib | Process management |
| `encoding/json` | stdlib | Task/result serialization |
| `encoding/base64` | stdlib | Firmware decoding |
| `regexp` | stdlib | Output parsing |

---

#go #microservice #patterns #queue-processing #context #error-handling #concurrency
