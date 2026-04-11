# ARM Cortex-M3 Architecture

> [!summary]
> The ARM Cortex-M3 is a 32-bit RISC processor core optimized for microcontroller applications, implementing the ARMv7-M architecture with Thumb-2 instruction set.

## Core Features

| Feature | Description |
|---------|-------------|
| **Architecture** | ARMv7-M |
| **Pipeline** | 3-stage (fetch, decode, execute) |
| **Instruction set** | Thumb-2 (16-bit and 32-bit mixed) |
| **Data path** | 32-bit |
| **Bus interface** | Harvard (separate I-code, D-code buses) |
| **Interrupt latency** | 12 cycles (deterministic) |
| **Endian** | Little-endian (configurable to big-endian) |

## Register File

### General-Purpose Registers

```
┌─────────────────────────────────────┐
│ R0  │  Function argument 1          │
│ R1  │  Function argument 2          │
│ R2  │  Function argument 3          │
│ R3  │  Function argument 4          │
│ R4  │  Callee-saved local           │
│ R5  │  Callee-saved local           │
│ R6  │  Callee-saved local           │
│ R7  │  Callee-saved / frame pointer │
│ R8  │  Local variable               │
│ R9  │  Local variable               │
│ R10 │  Local variable               │
│ R11 │  Frame pointer (FP)           │
│ R12 │  Intra-procedure scratch (IP) │
│ R13 │  Stack Pointer (SP)           │
│ R14 │  Link Register (LR)           │
│ R15 │  Program Counter (PC)         │
└─────────────────────────────────────┘
```

### Special Registers

| Register | Purpose |
|----------|---------|
| **xPSR** | Combined Program Status Register |
| **PRIMASK** | Priority mask (disable all configurable interrupts) |
| **FAULTMASK** | Fault mask (disable all except NMI) |
| **BASEPRI** | Base priority mask (mask by priority level) |
| **CONTROL** | Thread/handler mode, privileged/unprivileged, MSP/PSP select |

## Thumb-2 Instruction Set

Thumb-2 mixes 16-bit and 32-bit instructions for code density and performance:

### 16-bit Instructions (Thumb-16)

- Most data processing: MOV, ADD, SUB, CMP, AND, EOR
- Conditional branches: B.cond
- Load/store: LDR, STR (various addressing modes)
- Special: NOP, BKPT

### 32-bit Instructions (Thumb-32)

- Subroutine calls: BL, BLX
- Wide immediates: MOVW, MOVT
- DSP: SMLA, SMUL
- Exclusive access: LDREX, STREX

**Simulator support:** Currently only Thumb-16 (16-bit instructions). Thumb-32 requires 2-halfword decoding.

## Exception Model

### Exception Types

| Exception | IRQ | Priority | Description |
|-----------|-----|----------|-------------|
| **Reset** | -3 | -3 (highest) | Core reset |
| **NMI** | -2 | -2 | Non-maskable interrupt |
| **HardFault** | -1 | -1 | Unrecoverable error |
| **MemManage** | 4 | Configurable | Memory protection fault |
| **BusFault** | 5 | Configurable | Bus error |
| **UsageFault** | 6 | Configurable | Undefined instruction, unaligned access |
| **SVCall** | 11 | Configurable | Supervisor call |
| **PendSV** | 14 | Configurable | Pendable service |
| **SysTick** | 15 | Configurable | System tick timer |
| **IRQ 0+** | 16+ | Configurable | Peripheral interrupts |

### Exception Entry (Hardware)

```
Exception occurs
      │
      ▼
1. Complete current instruction
      │
      ▼
2. Push registers to stack (8 words):
   xPSR, PC, LR, R12, R3, R2, R1, R0
      │
      ▼
3. Read vector table entry for exception
      │
      ▼
4. Load PC from vector table
      │
      ▼
5. Set xPSR.IPSR to exception number
      │
      ▼
6. Execute ISR
```

### Exception Exit (Hardware)

```
ISR executes BX LR (where LR = EXC_RETURN)
      │
      ▼
1. Pop registers from stack
      │
      ▼
2. Restore PC, xPSR, LR
      │
      ▼
3. Return to interrupted code
```

### EXC_RETURN Values

| Value | Return Mode | Stack Pointer |
|-------|-------------|---------------|
| 0xFFFFFFF1 | Handler mode | MSP |
| 0xFFFFFFF9 | Thread mode | MSP |
| 0xFFFFFFFD | Thread mode | PSP |

## Pipeline Architecture

```
Fetch ──► Decode ──► Execute
  │          │          │
  │          │          │
  ▼          │          │
PC+4         │          │
             ▼          │
         Current        │
         instruction    │
                        ▼
                   Result written
                   Flags updated
```

**Key detail:** During instruction execution, PC = current instruction address + 4 (due to pipeline).

## Memory Ordering

Cortex-M3 uses **little-endian** byte ordering by default:

```
Word 0x12345678 stored at address 0x20000000:

Address    Byte
0x20000000  0x78  (LSB)
0x20000001  0x56
0x20000002  0x34
0x20000003  0x12  (MSB)
```

This matches the simulator's implementation in `memory.c`.

## Bit-Banding

Cortex-M3 supports **bit-banding** — mapping individual bits to word addresses for atomic manipulation:

```
Bit-band alias region:
  SRAM:  0x22000000 – 0x23FFFFFF
 Periph: 0x42000000 – 0x43FFFFFF

Formula:
  bit_word_offset = byte_offset × 32 + bit_number × 4
  bit_word_alias = bit_band_base + bit_word_offset

Example: Set bit 3 of SRAM word at 0x20000000
  Write to 0x2200000C (0x22000000 + 0×32 + 3×4)
```

**Simulator status:** ❌ Not implemented

## NVIC (Nested Vectored Interrupt Controller)

### Features

- **80 IRQ lines** (configurable priorities)
- **16 priority levels** (0–15, 0 = highest)
- **Priority grouping** (preempt priority vs sub-priority)
- **Tail-chaining** (back-to-back interrupt optimization)
- **Late arrival** (higher priority interrupt during stacking)

### Priority Grouping

```
Priority register (8 bits):
  [7:4]  Preempt priority  (which interrupts can preempt)
  [3:0]  Sub-priority      (ordering within same preempt level)

Example: AIRC.PRIGROUP = 0b011
  Preempt: 4 bits (16 levels)
  Sub:     0 bits (no sub-priority)
```

**Simulator status:** Priority grouping not implemented — flat 8-bit priority.

## Comparison with Other ARM Cores

| Feature | Cortex-M0 | Cortex-M3 | Cortex-M4 | Cortex-M7 |
|---------|-----------|-----------|-----------|-----------|
| **Architecture** | ARMv6-M | ARMv7-M | ARMv7E-M | ARMv7E-M |
| **Pipeline** | 3-stage | 3-stage | 3-stage | 6-stage |
| **Instructions** | Thumb-1 | Thumb-2 | Thumb-2 + DSP | Thumb-2 + DSP + FP |
| **Multiply** | No | Yes | Yes | Yes |
| **DSP** | No | No | Yes | Yes |
| **FPU** | No | No | Optional | Optional |
| **MPU** | Optional | Yes | Yes | Yes |
| **Cache** | No | No | No | Optional I/D |

---

#arm #cortex-m3 #armv7-m #thumb-2 #exception-model #pipeline #nvic #bit-banding
