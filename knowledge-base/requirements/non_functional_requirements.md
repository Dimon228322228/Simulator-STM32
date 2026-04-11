# Non-Functional Requirements

> [!summary]
> Non-functional requirements define quality attributes of the STM32 simulator system, including performance, reliability, scalability, and resource constraints.

## Performance Requirements

### NFR-1: Execution Speed

**Requirement:** "Типовое задание (мигание светодиодом, вывод строки по UART) должно выполняться не дольше 2 секунд реального времени на современном процессоре."

| Scenario | Max Real Time | Measurement |
|----------|--------------|-------------|
| LED blink (100 steps) | < 2 seconds | Wall clock time |
| UART string output (50 chars) | < 2 seconds | Wall clock time |
| Fibonacci (50 iterations) | < 2 seconds | Wall clock time |

**Current performance:** >1000 steps/second on modern CPU ✅

### NFR-2: Step Limit Enforcement

**Requirement:** The simulator shall terminate execution after a configurable number of steps to prevent infinite loops.

**Implementation:** `simulator_run(sim, max_steps)` checks step counter each iteration.

### NFR-3: Timeout Enforcement

**Requirement:** The orchestrator shall kill simulator processes that exceed the task timeout.

**Default timeout:** 5 seconds per task

**Implementation:** Go `exec.CommandContext` with timeout, process kill on expiration.

## Reliability Requirements

### NFR-4: Crash Isolation

**Requirement:** "При ошибках в моделируемом коде (деление на ноль, недопустимый адрес) симулятор не должен аварийно завершаться — необходимо выдавать диагностику и завершать задачу с ошибкой."

**Behavior:**
- Unknown instruction → Print error, set PC to halt address, exit gracefully
- Invalid memory access → Return default value (0xFF), continue execution
- Division by zero → Not yet implemented (would trigger HardFault exception)

### NFR-5: Resource Cleanup

**Requirement:** All allocated resources shall be freed on exit.

**Implementation:**
```c
memory_free(&sim.mem);  // Frees flash and sram arrays
return 0;               // Clean exit
```

**Orchestrator cleanup:**
```go
defer os.Remove(tmpFilename)  // Remove temp .bin file after execution
```

### NFR-6: Graceful Shutdown

**Requirement:** The orchestrator shall handle SIGINT/SIGTERM signals and shut down gracefully.

**Implementation:**
```go
sigChan := make(chan os.Signal, 1)
signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
go func() {
    sig := <-sigChan
    log.Printf("[INFO] Received signal %v, shutting down...", sig)
    cancel()  // Cancel context, stop processing
}()
```

## Scalability Requirements

### NFR-7: Concurrent Task

**Requirement:** "Микросервис должен поддерживать одновременную обработку не менее n задач за счёт запуска нескольких экземпляров ядра."

**Current implementation:** Sequential (one task at a time)

**Required enhancement:** Worker pool with N concurrent simulator processes

**Architecture:**
```
┌──────────────────────────────────────┐
│           Orchestrator                │
│                                      │
│  ┌─────────┐ ┌─────────┐ ┌────────┐ │
│  │Worker 1 │ │Worker 2 │ │Worker N│ │
│  │  (sim)  │ │  (sim)  │ │  (sim) │ │
│  └─────────┘ └─────────┘ └────────┘ │
└──────────────────────────────────────┘
```

### NFR-8: Horizontal Scaling

**Requirement:** Multiple orchestrator instances shall be able to consume from the same Redis queue without conflicts.

**Mechanism:** Redis BLPOP is atomic — only one consumer receives each task.

## Resource Constraints

### NFR-9: Memory Usage

**Requirement:** "Один экземпляр ядра — не более 50 МБ ОЗУ."

**Memory breakdown:**
| Component | Size |
|-----------|------|
| Flash | 64 KB |
| SRAM | 20 KB |
| Peripherals (structures) | ~10 KB |
| C runtime overhead | ~1 MB |
| **Total** | **~1.1 MB** ✅ |

**Orchestrator memory:**
| Component | Size |
|-----------|------|
| Go runtime | ~10 MB |
| Task/Result structures | ~100 KB |
| Stdout buffer | ~100 KB |
| **Total** | **~10.2 MB** ✅ |

### NFR-10: Disk Usage

| Component | Size |
|-----------|------|
| Simulator binary | < 1 MB |
| Orchestrator binary | < 10 MB |
| Docker image | < 100 MB |
| Temp .bin file (per task) | < 64 KB |

## Portability Requirements

### NFR-11: Platform Support

**Requirement:** "Целевая платформа для симулятора: Linux (Ubuntu 20.04+ / Linux Mint) или Windows 10+ с WSL2."

**Supported platforms:**
| Platform | Build System | Compiler | Status |
|----------|-------------|----------|--------|
| Linux (Ubuntu 20.04+) | CMake + make | GCC 9+ | ✅ Tested |
| Linux (WSL2) | CMake + make | GCC 9+ | ✅ Tested |
| Windows 10+ | CMake + MinGW | GCC (MinGW) | ✅ Tested |
| macOS | CMake + make | Clang | ⚠️ Not tested |

### NFR-12: Containerization

**Requirement:** The system shall be deployable as a Docker container.

**Dockerfile features:**
- Multi-stage build (3 stages)
- Final image: debian-slim (minimal footprint)
- Includes both simulator and orchestrator

```bash
docker build -t stm32-simulator:latest .
```

## Maintainability Requirements

### NFR-13: Code Structure

**Requirement:** The codebase shall follow a modular architecture with clear separation of concerns.

**Structure:**
```
core/
├── include/     # Public headers (API contracts)
└── src/         # Implementation files
orchestrator/
├── types/       # Data type definitions
├── keydb/       # Redis client wrapper
└── simulator/   # Simulator runner and parser
test/            # Test source files
```

### NFR-14: Documentation

**Requirement:** The project shall include:
- README.md with architecture overview
- GETTING_STARTED.md with build/run instructions
- Technical specification (ТЗ) document
- Knowledge base (Obsidian markdown files)

### NFR-15: Testing

**Requirement:** The project shall include unit, integration, and system tests.

**Coverage goals:**
- Core modules (memory, CPU, execution): > 70%
- Peripherals: > 50%
- Orchestrator: > 50%

## Security Requirements

### NFR-16: Input Validation

**Requirement:** The orchestrator shall validate all inputs before processing.

**Validations:**
- Base64 decoding failure → Error result
- File size exceeds Flash capacity → Rejected
- Invalid JSON task → Parse error
- Simulator path doesn't exist → Startup error

### NFR-17: Process Isolation

**Requirement:** Each task shall execute in an isolated process to prevent cross-task interference.

**Implementation:**
- New process per task (`exec.Command`)
- Temp file with unique name (`os.CreateTemp`)
- Process killed on timeout (no lingering processes)

### NFR-18: No Secret Exposure

**Requirement:** Logs and results shall not contain sensitive information.

**Current status:** ✅ No secrets used (no API keys, passwords, tokens)

## Observability Requirements

### NFR-19: Logging

**Requirement:** The orchestrator shall log task lifecycle events.

**Log format:**
```
[INFO] Starting STM32 Orchestrator
[INFO] Received task task_001 (student: student_42, lab: 1)
[INFO] Task task_001 completed in 1.234s
[ERROR] Failed to run task task_002: timeout
```

### NFR-20: Metrics (Future)

**Requirement:** "Интегрировать сбор метрик и трейсов через OpenTelemetry."

**Planned metrics:**
- Task processing time (histogram)
- Tasks processed (counter, success/error/timeout)
- Simulator execution time (histogram)
- Queue depth (gauge)

**Status:** ❌ Not yet implemented

---

#non-functional #performance #reliability #scalability #memory #portability #security
