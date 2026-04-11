# Overall Architecture

> [!summary]
> High-level architecture of the STM32F103C8T6 simulator system, describing component interactions, deployment topology, and design principles.

## Responsibility

The architecture defines a **two-tier system**:
1. **C Simulator Core** — standalone executable that models the Cortex-M3 CPU, memory, and peripherals
2. **Go Orchestrator** — microservice that manages task lifecycle, integrates with KeyDB/Redis queues, and invokes the C simulator as a subprocess

## Design Principles

### Separation of Concerns
- **C core** focuses purely on hardware emulation (CPU cycles, memory maps, peripheral registers)
- **Go orchestrator** handles business logic (task queues, timeouts, result parsing, metrics)
- Communication between them is **unidirectional**: orchestrator → simulator (stdin/CLI) → orchestrator (stdout parsing)

### Process Isolation
Each task execution spawns a **new process** of the C simulator. This ensures:
- No state leakage between tasks
- Crash isolation — a segfault in one task doesn't affect the orchestrator
- Easy horizontal scaling (multiple orchestrator instances)

### Deterministic Execution
The simulator uses a **step counter** (`max_steps`) instead of real-time clocking. This guarantees:
- Reproducible results across runs
- No dependency on host CPU speed
- Protection against infinite loops (common in student code with `B .` loops)

## Component Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                        External World                         │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────────┐  │
│  │ ITMO.clab    │   │ Student CLI  │   │ Test Framework   │  │
│  │ (Web UI)     │   │ (manual run) │   │ (CI/CD)          │  │
│  └──────┬───────┘   └──────┬───────┘   └────────┬─────────┘  │
│         │                  │                     │            │
│         ▼                  ▼                     ▼            │
│  ┌──────────────────────────────────────────────────────┐    │
│  │              Go Orchestrator                          │    │
│  │                                                       │    │
│  │  ┌────────────┐  ┌──────────────┐  ┌──────────────┐  │    │
│  │  │ keydb/     │  │ simulator/   │  │ types/       │  │    │
│  │  │ client.go  │  │ runner.go    │  │ types.go     │  │    │
│  │  └────────────┘  └──────────────┘  └──────────────┘  │    │
│  └──────────────────────┬───────────────────────────────┘    │
│                         │ exec.Command()                     │
│                         │ CLI args + .bin file               │
│                         ▼                                    │
│  ┌──────────────────────────────────────────────────────┐    │
│  │              C Simulator Core                         │    │
│  │                                                       │    │
│  │  main.c ──► simulator_run() ──► simulator_step()     │    │
│  │                                                       │    │
│  │  ┌──────┐ ┌───────┐ ┌─────┐ ┌────┐ ┌────┐ ┌──────┐  │    │
│  │  │ CPU  │ │Memory │ │GPIO │ │TIM6│ │NVIC│ │USART │  │    │
│  │  └──────┘ └───────┘ └─────┘ └────┘ └────┘ └──────┘  │    │
│  │  ┌────┐ ┌────┐ ┌────┐ ┌─────────┐                   │    │
│  │  │SPI │ │I2C │ │DMA │ │BusMatrix│                   │    │
│  │  └────┘ └────┘ └────┘ └─────────┘                   │    │
│  │                                                       │    │
│  │  Output: stdout (register state, stats, GPIO logs)   │    │
│  └──────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
```

## Memory Map

The simulator implements the STM32F103C8T6 memory map as defined in [[memory_model\|Memory Model]]:

| Region | Address Range | Size | Access |
|--------|--------------|------|--------|
| **Flash** | `0x08000000` – `0x0800FFFF` | 64 KB | Read (execute), Write (load only) |
| **SRAM** | `0x20000000` – `0x20004FFF` | 20 KB | Read/Write |
| **Peripherals** | `0x40000000` – `0x4001FFFF` | 2 MB | Read/Write (register-level) |
| **NVIC** | `0xE000E100` – ... | — | Read/Write |

## Execution Flow

1. **Initialization** (`main.c`):
   - Allocate Flash and SRAM (`memory_init`)
   - Initialize all peripheral structures (`gpio_init`, `tim6_init`, `nvic_init`, etc.)
   - Reset CPU state (`cpu_reset`)
   - Load firmware into Flash (from `.bin` file or built-in demo)

2. **Fetch-Decode-Execute Loop** (`execute.c`):
   - `simulator_run()` loops until:
     - PC leaves Flash region
     - PC reaches halt address (`0xFFFFFFFF`)
     - Step limit reached
   - Each iteration calls `simulator_step()`:
     - **FETCH**: Read 16-bit Thumb instruction from `PC`
     - **DECODE**: Extract opcode (bits 15–11)
     - **EXECUTE**: Perform operation, update registers/memory, advance PC

3. **Termination**:
   - Print final CPU register state
   - Print execution statistics
   - Free memory and exit

## Inter-Component Communication

### C Simulator → Go Orchestrator
The simulator outputs structured text to stdout:
```
[INIT] Loaded 8 bytes from firmware.bin into Flash at 0x08000000
[GPIO] PORT_A_ODR = 0x00000001
[CPU] Final register state:
  R0:  0x00000005    ...
[STATS] Instructions executed: 4
[STATS] CPU cycles: 4
```

The orchestrator parses this output using regex patterns (`runner.go::parseOutput`).

### Go Orchestrator → C Simulator
CLI arguments:
```bash
stm32_sim --max-steps 10000 /tmp/firmware_abc123.bin
```

## Subsystem Index

| Subsystem | Documentation | Implementation |
|-----------|--------------|----------------|
| [[cpu_core\|CPU Core]] | `architecture/cpu_core.md` | `core/include/cpu_state.h`, `core/src/cpu_state.c` |
| [[memory_model\|Memory]] | `architecture/memory_model.md` | `core/include/memory.h`, `core/src/memory.c` |
| [[gpio\|GPIO]] | `architecture/gpio.md` | `core/include/gpio.h`, `core/src/gpio.c` |
| [[timer\|TIM6]] | `architecture/timer.md` | `core/include/tim6.h` |
| [[uart\|UART]] | `architecture/uart.md` | `core/include/usart.h` |
| [[interrupt_controller\|NVIC]] | `architecture/interrupt_controller.md` | `core/include/nvic.h` |
| [[instruction_execution\|Instructions]] | `implementation/instruction_execution.md` | `core/src/execute.c` |
| [[microservice\|Orchestrator]] | `architecture/microservice.md` | `orchestrator/main.go` |

---

#architecture #system-design #stm32 #orchestrator
