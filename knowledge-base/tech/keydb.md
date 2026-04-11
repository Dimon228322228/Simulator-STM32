# KeyDB / Redis

> [!summary]
> KeyDB (a Redis-compatible fork) serves as the task queue and result store for the STM32 simulator orchestrator, providing reliable, high-throughput message passing between the cloud lab and the simulator.

## What is KeyDB?

KeyDB is a **multi-threaded fork of Redis** with:
- Same API and protocol (RESP)
- Higher throughput (multi-threaded vs Redis single-threaded)
- Active Active Replication (multi-master)
- Drop-in replacement for Redis

**For the orchestrator:** KeyDB is functionally identical to Redis — any Redis client works.

## Queue Protocol

### Task Submission

```
ITMO.clab ──LPUSH──► simulator:tasks
```

```bash
redis-cli LPUSH simulator:tasks '{
  "task_id": "task_001",
  "student_id": "student_42",
  "lab_number": 1,
  "binary": "BQUDGII=",
  "timeout_sec": 5
}'
```

### Task Consumption

```
Orchestrator ──BLPOP──► simulator:tasks
```

```go
result, err := redis.BLPop(ctx, timeout, "simulator:tasks").Result()
// result[0] = "simulator:tasks" (queue name)
// result[1] = JSON task string
```

**BLPOP behavior:**
- Blocks until element available OR timeout expires
- Returns `nil` on timeout (not error)
- Atomically removes element from queue

### Result Submission

```
Orchestrator ──RPUSH──► simulator:results
```

```go
data, _ := json.Marshal(result)
redis.RPush(ctx, "simulator:results", data).Err()
```

### Result Retrieval

```
ITMO.clab ──RPOP──► simulator:results
```

```bash
redis-cli RPOP simulator:results
```

## Queue Semantics

### LPUSH + BLPOP = FIFO Queue

```
Producer: LPUSH task1, LPUSH task2, LPUSH task3

Queue state: [task3, task2, task1]  ← task1 at tail

Consumer: BLPOP → task1 (first in, first out)
          BLPOP → task2
          BLPOP → task3
```

### RPUSH + RPOP = FIFO Queue (for results)

```
Orchestrator: RPUSH result1, RPUSH result2

Queue state: [result1, result2]

Server: RPOP → result1 (first in, first out)
        RPOP → result2
```

## Client Implementation (`keydb/client.go`)

### Connection

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
    
    // Test connection (non-blocking on failure)
    if err := rdb.Ping(ctx).Err(); err != nil {
        log.Printf("[WARN] Cannot connect to KeyDB: %v", err)
    }
    
    return &Client{redis: rdb, ctx: context.Background()}
}
```

### Pop Task (Blocking)

```go
func (c *Client) PopTask(queueName string, timeout time.Duration) (*Task, error) {
    result, err := c.redis.BLPop(c.ctx, timeout, queueName).Result()
    if err == redis.Nil {
        return nil, nil  // Timeout — queue empty
    }
    if err != nil {
        return nil, fmt.Errorf("failed to pop task: %w", err)
    }
    
    if len(result) < 2 {
        return nil, fmt.Errorf("invalid response from Redis")
    }
    
    var task Task
    if err := json.Unmarshal([]byte(result[1]), &task); err != nil {
        return nil, fmt.Errorf("failed to unmarshal task: %w", err)
    }
    
    return &task, nil
}
```

### Pop Task (Non-Blocking)

```go
func (c *Client) PopTaskNoBlock(queueName string) (*Task, error) {
    result, err := c.redis.LPop(c.ctx, queueName).Result()
    if err == redis.Nil {
        return nil, nil  // Queue empty
    }
    // ... same unmarshal logic
}
```

### Push Result

```go
func (c *Client) PushResult(resultQueueName string, result *TaskResult) error {
    data, err := json.Marshal(result)
    if err != nil {
        return fmt.Errorf("failed to marshal result: %w", err)
    }
    
    if err := c.redis.RPush(c.ctx, resultQueueName, data).Err(); err != nil {
        return fmt.Errorf("failed to push result: %w", err)
    }
    
    return nil
}
```

## Deployment

### Local Development

```bash
# Run Redis in Docker
docker run -d -p 6379:6379 redis:latest

# Test connection
redis-cli ping
# → PONG

# Push test task
redis-cli LPUSH simulator:tasks '{"task_id":"test","binary":"BQUDGII=","timeout_sec":5}'

# Run orchestrator
./orchestrator --mode queue --redis localhost:6379

# Check result
redis-cli RPOP simulator:results
```

### Production (KeyDB)

```bash
# Docker Compose
version: '3'
services:
  keydb:
    image: eqalpha/keydb:latest
    ports:
      - "6379:6379"
    command: keydb-server --appendonly yes
  
  orchestrator:
    build: .
    command: ./orchestrator --mode queue --redis keydb:6379
    depends_on:
      - keydb
```

## Redis Commands Used

| Command | Usage | Purpose |
|---------|-------|---------|
| `PING` | Connection test | Verify connectivity |
| `LPUSH` | ITMO.clab → queue | Add task to head |
| `BLPOP` | Orchestrator ← queue | Blocking task removal |
| `LPOP` | Orchestrator ← queue | Non-blocking check |
| `RPUSH` | Orchestrator → queue | Add result to tail |
| `RPOP` | ITMO.clab ← queue | Retrieve result |

## Data Persistence

### Default: No Persistence

Redis/KeyDB stores data in memory. On restart, all data is lost.

**For the simulator:** This is acceptable — tasks are transient and should be re-submitted if lost.

### Optional: AOF (Append-Only File)

```bash
# Enable in redis.conf
appendonly yes
appendfilename "appendonly.aof"
```

**Trade-off:** Durability vs. performance (AOF writes every command to disk).

## Scaling Considerations

### Single Queue, Multiple Consumers

```
┌─────────────┐
│   Redis     │
│ simulator:  │
│   tasks     │
└──┬──┬──┬──┬─┘
   │  │  │  │
   ▼  ▼  ▼  ▼
 ┌──┐┌──┐┌──┐┌──┐
 │O1││O2││O3││O4│  ← 4 orchestrator instances
 └──┘└──┘└──┘└──┘
```

**BLPOP guarantees:** Each task goes to exactly one consumer (atomic pop).

### Queue Monitoring

```bash
# Check queue depth
redis-cli LLEN simulator:tasks

# Check result queue depth
redis-cli LLEN simulator:results

# Watch in real-time
redis-cli MONITOR | grep "simulator:"
```

## Alternatives Considered

| Technology | Pros | Cons | Decision |
|-----------|------|------|----------|
| **Redis/KeyDB** | Simple, well-supported, atomic operations | In-memory only (by default) | ✅ **Chosen** |
| RabbitMQ | Routing, acknowledgments, durable | Complex setup, heavier | ❌ Overkill |
| Apache Kafka | High throughput, persistent | Overkill for this use case | ❌ Too complex |
| SQLite file queue | Simple, no external deps | No concurrent consumers | ❌ Limited |

---

#keydb #redis #queue #message-broker #blpop #rpush #task-queue
