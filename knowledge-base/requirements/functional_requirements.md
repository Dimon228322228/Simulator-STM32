# Functional Requirements

> [!summary]
> Functional requirements define what the STM32 simulator system must do, organized by component and aligned with the technical specification (ТЗ).

## Core Simulator Requirements

### FR-1: Firmware Loading

**Priority:** 🔴 Critical

The simulator shall:
- Load binary `.bin` files into simulated Flash memory at address `0x08000000`
- Validate file size does not exceed Flash capacity (64 KB)
- Initialize the Program Counter to the Flash base address
- Provide a built-in demo mode that doesn't require external files

**Acceptance criteria:**
- ✅ 8-byte demo program loads and executes without file
- ✅ 64 KB file loads successfully
- ✅ 65 KB file is rejected with error message

### FR-2: Instruction Execution

**Priority:** 🔴 Critical

The simulator shall execute Thumb-16 instructions according to ARMv7-M architecture:

#### FR-2.1: Data Processing

| Instruction | Format | Status |
|------------|--------|--------|
| **MOVS** | Rd, #imm8 | ✅ Implemented |
| **ADDS** | Rd, Rn, #imm3 / Rd, #imm8 / Rd, Rn, Rm | ✅ Implemented |
| **SUBS** | Rd, Rn, #imm3 / Rd, #imm8 / Rd, Rn, Rm | ✅ Implemented |
| **CMP** | Rn, Rm | ✅ Implemented |
| **ANDS** | Rd, Rm | ✅ Implemented |
| **EORS** | Rd, Rm | ✅ Implemented |
| **LSLS** | Rd, Rm | ✅ Implemented |
| **NEGS** | Rd, Rm | ✅ Implemented |
| **ADCS** | Rd, Rn, Rm | ✅ Implemented |
| **SBCS** | Rd, Rn, Rm | ✅ Implemented |
| **RORS** | Rd, Rn, Rm | ✅ Implemented |
| **MULS** | Rd, Rn, Rm | ✅ Implemented |

#### FR-2.2: Memory Access

| Instruction | Format | Status |
|------------|--------|--------|
| **LDR** | Rd, [Rn, Rm] / Rd, [Rn, #imm5] / Rd, [SP, #imm8] / Rd, [PC, #imm8] | ✅ Implemented |
| **STR** | Rd, [Rn, Rm] / Rd, [Rn, #imm5] / Rd, [SP, #imm8] | ✅ Implemented |
| **LDRB** | Rd, [Rn, Rm] / Rd, [Rn, #imm5] | ✅ Implemented |
| **STRB** | Rd, [Rn, Rm] / Rd, [Rn, #imm5] | ✅ Implemented |
| **LDRH** | Rd, [Rn, Rm] / Rd, [Rn, #imm5] | ✅ Implemented |
| **STRH** | Rd, [Rn, Rm] / Rd, [Rn, #imm5] | ✅ Implemented |

#### FR-2.3: Branching

| Instruction | Format | Status |
|------------|--------|--------|
| **B** | label (unconditional) | ✅ Implemented |
| **B.cond** | label (conditional, 16 conditions) | ✅ Implemented |
| **BX** | Rm | ✅ Implemented |
| **BL/BLX** | label (32-bit) | ❌ Stub (not supported) |

### FR-3: Flag Management

**Priority:** 🔴 Critical

The simulator shall maintain xPSR flags and update them after arithmetic operations:

| Flag | Name | Update Condition |
|------|------|-----------------|
| **N** | Negative | Result bit 31 = 1 |
| **Z** | Zero | Result = 0 |
| **C** | Carry | Unsigned overflow (addition: result < operand; subtraction: operand1 ≥ operand2) |
| **V** | Overflow | Signed overflow (operands have same sign, result has different sign) |

### FR-4: Memory Subsystem

**Priority:** 🔴 Critical

The simulator shall provide:
- **Flash**: 64 KB at `0x08000000` (read-only during execution)
- **SRAM**: 20 KB at `0x20000000` (read/write)
- **Little-endian** byte ordering for multi-byte accesses
- Byte, halfword, and word access functions

### FR-5: GPIO Emulation

**Priority:** 🟡 High

The simulator shall emulate 7 GPIO ports (A–G) with:
- CRL/CRH configuration registers
- IDR (input) and ODR (output) data registers
- BSRR atomic bit set/reset
- BRR bit reset
- Logging of ODR changes to stdout

### FR-6: Timer Emulation (TIM6)

**Priority:** 🟡 High

The simulator shall emulate TIM6 basic timer with:
- CR1 control register (CEN, UDIS, URS, OPM, ARPE bits)
- 16-bit CNT counter
- 16-bit PSC prescaler
- 16-bit ARR auto-reload register
- Update event generation (UIF flag)
- Interrupt request when UIF=1 and UIE=1

### FR-7: UART Emulation

**Priority:** 🟡 High

The simulator shall emulate USART1/2/3 with:
- SR status register (TXE, RXNE, TC flags)
- DR data register
- BRR baud rate register
- CR1 control register (TE, RE, UE, interrupt enables)
- TX output logged to stdout as `[UART] TX: 0xXX`
- RX input capability (byte injection from test environment)

### FR-8: Interrupt Controller (NVIC)

**Priority:** 🟡 High

The simulator shall emulate NVIC with:
- ISER/ICER (interrupt enable/disable)
- ISPR/ICPR (interrupt pending management)
- IPR (interrupt priority registers, 80 IRQs)
- IABR (active bit tracking)
- Integration with CPU for exception entry/exit (stacking, vector lookup, EXC_RETURN)

## Orchestrator Requirements

### FR-9: Queue Mode Operation

**Priority:** 🔴 Critical

The orchestrator shall:
- Connect to KeyDB/Redis server
- Block-pop tasks from `simulator:tasks` queue
- Execute each task via C simulator subprocess
- Push results to `simulator:results` queue
- Handle connection failures gracefully (log warning, continue)

### FR-10: File Mode Operation

**Priority:** 🟡 High

The orchestrator shall:
- Load task from JSON file
- Execute via C simulator
- Write result to JSON file or stdout
- Support separate binary file (if not embedded in task)

### FR-11: Task Execution

**Priority:** 🔴 Critical

For each task, the orchestrator shall:
- Decode base64-encoded firmware binary
- Create temporary file for simulator
- Launch simulator with `--max-steps` argument
- Enforce timeout (configurable per task, default 5 seconds)
- Kill simulator process on timeout
- Parse stdout for: statistics, GPIO state, UART output, register values
- Return structured result with status ("success", "error", "timeout")

### FR-12: Output Parsing

**Priority:** 🔴 Critical

The orchestrator shall extract from simulator stdout:
- Instructions executed count: `[STATS] Instructions executed: N`
- CPU cycles: `[STATS] CPU cycles: N`
- GPIO state: `[GPIO] PORT_X_ODR = 0xXXXXXXXX`
- UART bytes: `[UART] TX: 0xXX`
- Register values: `R0:  0xXXXXXXXX`, etc.
- Error messages: `[ERROR] ...`

## Interface Requirements

### FR-13: CLI Interface

**Priority:** 🔴 Critical

The simulator shall accept command-line arguments:

| Argument | Description | Default |
|----------|-------------|---------|
| `[firmware.bin]` | Path to firmware file | None (demo mode) |
| `--help` | Show usage information | — |
| `--demo` | Run built-in demo program | Off |
| `--max-steps N` | Limit instruction execution | 100 |
| `--verbose` | Enable detailed logging | Off |

### FR-14: Output Format

**Priority:** 🔴 Critical

The simulator shall output structured information to stdout:

```
[INIT] Vector table initialized (MSP=0x20005000, PC=0x08000008)
[INIT] Loaded N bytes from firmware.bin into Flash at 0x08000000
[GPIO] PORT_X_ODR = 0xXXXXXXXX  (when ODR changes)
[UART] TX: 0xXX  (when byte transmitted)

[CPU] Final register state:
  R0:  0xXXXXXXXX    R4:  0xXXXXXXXX    ...
  ...

========================================
       SIMULATION STATISTICS
========================================
[STATS] Instructions executed: N
[STATS] CPU cycles: N
[STATS] UART bytes sent: N
========================================
```

---

#requirements #functional #features #instructions #peripherals #orchestrator
