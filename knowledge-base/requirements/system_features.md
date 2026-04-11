# System Features

> [!summary]
> Complete feature inventory of the STM32 simulator system, organized by component and implementation status.

## Feature Inventory

### Simulator Core (C)

| ID | Feature | Component | Status | Description |
|----|---------|-----------|--------|-------------|
| F-001 | Binary file loading | main.c | ✅ | Load .bin files into Flash |
| F-002 | Demo mode | main.c | ✅ | Built-in 4-instruction program |
| F-003 | CLI argument parsing | main.c | ✅ | --help, --demo, --max-steps, --verbose |
| F-004 | Flash memory (64 KB) | memory.c | ✅ | Read-only during execution |
| F-005 | SRAM memory (20 KB) | memory.c | ✅ | Read/write access |
| F-006 | Little-endian byte ordering | memory.c | ✅ | Halfword/word access |
| F-007 | CPU register file | cpu_state.c | ✅ | R0-R15 + special registers |
| F-008 | CPU reset | cpu_state.c | ✅ | Zero all registers |
| F-009 | Thumb-16 fetch | execute.c | ✅ | Read 16-bit instruction, PC+=2 |
| F-010 | Opcode decoding | execute.c | ✅ | Extract bits [15:11] |
| F-011 | MOVS Rd, #imm8 | execute.c | ✅ | Load 8-bit immediate |
| F-012 | ADDS (all variants) | execute.c | ✅ | Immediate 3/8-bit, register |
| F-013 | SUBS (all variants) | execute.c | ✅ | Immediate 3/8-bit, register |
| F-014 | CMP Rn, Rm | execute.c | ✅ | Compare, flags only |
| F-015 | ANDS/EORS/LSLS/NEGS | execute.c | ✅ | Extended ALU group |
| F-016 | ADCS/SBCS/RORS/MULS | execute.c | ✅ | Multiply/shift group |
| F-017 | LDR (all addressing modes) | execute.c | ✅ | Register, immediate, PC, SP |
| F-018 | STR (all addressing modes) | execute.c | ✅ | Register, immediate, SP |
| F-019 | LDRB/STRB | execute.c | ✅ | Byte load/store |
| F-020 | LDRH/STRH | execute.c | ✅ | Halfword load/store |
| F-021 | B (unconditional) | execute.c | ✅ | 11-bit signed offset × 2 |
| F-022 | B.cond (16 conditions) | execute.c | ✅ | 8-bit signed offset × 2 |
| F-023 | BX Rm | execute.c | ✅ | Branch indirect |
| F-024 | N/Z flag update | execute.c | ✅ | After arithmetic |
| F-025 | C/V flag update | execute.c | ✅ | Carry and overflow |
| F-026 | Sign extension | execute.c | ✅ | For branch offsets |
| F-027 | Step limit enforcement | execute.c | ✅ | Prevent infinite loops |
| F-028 | Register state output | main.c | ✅ | Print all R0-R15 at end |
| F-029 | Statistics output | main.c | ✅ | Instructions, cycles, UART |
| F-030 | GPIO port initialization | gpio.c | ✅ | 7 ports (A-G) |
| F-031 | GPIO register read/write | gpio.c | ✅ | CRL, CRH, IDR, ODR, BSRR, BRR |
| F-032 | GPIO ODR logging | gpio.c | ✅ | Print on ODR change |
| F-033 | TIM6 initialization | tim6.h | ✅ | Register structure defined |
| F-034 | TIM6 register access | tim6.h | ✅ | Read/write functions |
| F-035 | NVIC initialization | nvic.h | ✅ | 80 IRQ support |
| F-036 | NVIC enable/disable | nvic.h | ✅ | ISER/ICER operations |
| F-037 | NVIC pending management | nvic.h | ✅ | ISPR/ICPR operations |
| F-038 | NVIC priority storage | nvic.h | ✅ | IPR array |
| F-039 | USART initialization | usart.h | ✅ | 3 instances, buffers |
| F-040 | USART register structure | usart.h | ✅ | SR, DR, BRR, CR1-3, GTPR |
| F-041 | SPI initialization | spi.h | ✅ | 3 instances, buffers |
| F-042 | SPI register structure | spi.h | ✅ | CR1-2, SR, DR, CRC |
| F-043 | I2C initialization | i2c.h | ✅ | 2 instances, buffers |
| F-044 | I2C register structure | i2c.h | ✅ | CR1-2, SR1-2, DR, CCR |
| F-045 | DMA initialization | dma.h | ✅ | 7 channels |
| F-046 | DMA register structure | dma.h | ✅ | ISR, IFCR, CCR, CNDTR, CPAR, CMAR |
| F-047 | RCC initialization | rcc.h | ✅ | Clock control registers |
| F-048 | RCC register structure | rcc.h | ✅ | CR, CFGR, CIR, enable registers |
| F-049 | Bus Matrix initialization | bus_matrix.h | ✅ | Address tracking |
| F-050 | Bus Matrix structure | bus_matrix.h | ✅ | RCC/DMA shadow registers |

### Orchestrator (Go)

| ID | Feature | Component | Status | Description |
|----|---------|-----------|--------|-------------|
| O-001 | CLI flag parsing | main.go | ✅ | All configuration options |
| O-002 | Queue mode | main.go | ✅ | BLPOP from Redis |
| O-003 | File mode | main.go | ✅ | JSON file input/output |
| O-004 | Signal handling | main.go | ✅ | SIGINT/SIGTERM graceful shutdown |
| O-005 | KeyDB client | keydb/client.go | ✅ | Connect, pop, push |
| O-006 | Task structure | types/types.go | ✅ | TaskID, Binary, Timeout, Config |
| O-007 | Result structure | types/types.go | ✅ | Status, GPIO, UART, stats |
| O-008 | Base64 decoding | simulator/runner.go | ✅ | Firmware extraction |
| O-009 | Temp file management | simulator/runner.go | ✅ | Create, write, cleanup |
| O-010 | Process spawning | simulator/runner.go | ✅ | exec.Command |
| O-011 | Timeout enforcement | simulator/runner.go | ✅ | Kill on expiration |
| O-012 | Stdout capture | simulator/runner.go | ✅ | strings.Builder |
| O-013 | Stats parsing | simulator/runner.go | ✅ | Regex extraction |
| O-014 | GPIO state parsing | simulator/runner.go | ✅ | PORT_X_ODR regex |
| O-015 | UART output parsing | simulator/runner.go | ✅ | TX byte regex |
| O-016 | Register value parsing | simulator/runner.go | ✅ | R0-R3 regex |
| O-017 | Error detection | simulator/runner.go | ✅ | [ERROR] pattern |
| O-018 | JSON serialization | main.go | ✅ | MarshalIndent for output |
| O-019 | JSON deserialization | main.go | ✅ | Unmarshal task from file |

### Missing Features (Planned)

| ID | Feature | Priority | Description |
|----|---------|----------|-------------|
| M-001 | BL/BLX (32-bit) | 🔴 High | Subroutine call with link |
| M-002 | PUSH/POP | 🔴 High | Stack operations |
| M-003 | MSR/MRS | 🔴 High | Special register access |
| M-004 | NOP | 🟢 Low | No-operation |
| M-005 | BKPT | 🟢 Low | Breakpoint |
| M-006 | CBZ/CBNZ | 🟡 Medium | Compare and branch on zero |
| M-007 | IT block | 🟡 Medium | If-Then conditional execution |
| M-008 | Peripheral bus routing | 🔴 Critical | Route memory writes to GPIO/UART/TIM6 |
| M-009 | Exception entry/exit | 🔴 Critical | Stacking, vector lookup, EXC_RETURN |
| M-010 | TIM6 counter update integration | 🟡 High | Call tim6_update_counter from main loop |
| M-011 | USART transmit trigger | 🟡 High | Output on DR write |
| M-012 | USART receive injection | 🟡 Medium | Inject bytes from test environment |
| M-013 | OpenTelemetry metrics | 🟢 Low | Task duration, error rates |
| M-014 | Concurrent worker pool | 🟡 Medium | Parallel task execution |
| M-015 | GPIO input simulation | 🟡 Medium | Inject pin states from test environment |
| M-016 | Bit-banding | 🟢 Low | Atomic bit manipulation via alias region |
| M-017 | MPU emulation | 🟢 Low | Memory protection unit |
| M-018 | Flash programming | 🟢 Low | Flash write/erase sequences |

## Feature Coverage by Lab Scenario

### Lab 1: Basic Arithmetic

| Required Feature | Status |
|-----------------|--------|
| MOV, ADD, SUB | ✅ |
| CMP, B.cond | ✅ |
| Register output | ✅ |
| Statistics | ✅ |

**Verdict:** ✅ Fully supported

### Lab 2: Memory Operations

| Required Feature | Status |
|-----------------|--------|
| LDR [PC, #imm] | ✅ |
| LDR [Rn, #imm] | ✅ |
| STR [Rn, #imm] | ✅ |
| SRAM read/write | ✅ |

**Verdict:** ✅ Supported (Flash/SRAM only, peripheral routing missing)

### Lab 3: GPIO Control

| Required Feature | Status |
|-----------------|--------|
| STR to GPIO address | ❌ (bus routing missing) |
| ODR logging | ✅ |
| BSRR atomic set/reset | ✅ |

**Verdict:** ⚠️ Partially supported (GPIO logic works, but writes don't reach module)

### Lab 4: Timer Interrupts

| Required Feature | Status |
|-----------------|--------|
| TIM6 counter | ❌ (not integrated) |
| Overflow detection | ❌ (update logic not called) |
| NVIC pending | ❌ (integration missing) |
| Exception entry | ❌ (not implemented) |

**Verdict:** ❌ Not supported

### Lab 5: UART Communication

| Required Feature | Status |
|-----------------|--------|
| DR write → output | ❌ (transmit not triggered) |
| TXE flag | ❌ (not managed) |
| RXNE injection | ❌ (no input mechanism) |

**Verdict:** ❌ Not supported

### Lab 6: Advanced (SPI/I2C/DMA)

| Required Feature | Status |
|-----------------|--------|
| SPI register access | ✅ (structure defined) |
| I2C register access | ✅ (structure defined) |
| DMA transfer | ❌ (no logic) |

**Verdict:** ❌ Not supported (structures only)

## Feature Implementation Effort

| Feature | Estimated Complexity | Dependencies |
|---------|---------------------|--------------|
| Peripheral bus routing | Medium (2-4 hours) | Memory module refactoring |
| Exception entry/exit | High (8-12 hours) | NVIC, vector table, stacking logic |
| BL/BLX | Medium (2-3 hours) | 32-bit instruction decoding |
| PUSH/POP | Medium (2-3 hours) | Stack pointer management |
| TIM6 integration | Low (1-2 hours) | Bus routing, main loop modification |
| USART transmit | Low (1-2 hours) | Bus routing, DR write handler |
| OpenTelemetry | Medium (4-6 hours) | Go dependencies, metric definitions |
| Worker pool | Medium (3-5 hours) | Goroutine management, error handling |

---

#features #feature-inventory #coverage #lab-scenarios #implementation-status
