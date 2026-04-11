# CPU Core

> [!summary]
> The CPU Core models the ARM Cortex-M3 processor, including 16 general-purpose registers, special system registers, and two stack pointers (MSP/PSP).

## Responsibility

The CPU Core is responsible for:
- Maintaining the **register file** (R0–R15, xPSR, PRIMASK, FAULTMASK, BASEPRI, CONTROL)
- Managing the **Program Counter (PC)** — R15
- Tracking **stack pointers** — Main Stack Pointer (MSP) and Process Stack Pointer (PSP)
- Providing a **reset mechanism** (`cpu_reset`)
- Integrating with the [[instruction_execution\|instruction executor]] to decode and execute Thumb-16 instructions

## Register File

### General-Purpose Registers (R0–R12)

| Register | Alias | Usage |
|----------|-------|-------|
| R0–R3 | Lo registers | Function parameters, return values, scratch registers |
| R4–R7 | Hi registers | Local variables, callee-saved |
| R8–R12 | Hi registers | Additional local variables |

### Special Registers (R13–R15)

| Register | Alias | Purpose |
|----------|-------|---------|
| R13 | SP (Stack Pointer) | Points to current stack location. In Thumb mode, must be word-aligned. |
| R14 | LR (Link Register) | Stores return address after BL/BLX instructions |
| R15 | PC (Program Counter) | Address of next instruction to fetch. In Thumb mode, bit 0 is always 0. |

### System Registers

| Register | Width | Purpose |
|----------|-------|---------|
| **xPSR** | 32-bit | Program Status Register — combines APSR, IPSR, EPSR |
| **PRIMASK** | 32-bit | Interrupt mask — when set, disables all exceptions with configurable priority |
| **FAULTMASK** | 32-bit | Fault mask — disables all exceptions except NMI |
| **BASEPRI** | 32-bit | Base priority mask — masks interrupts with priority ≤ BASEPRI |
| **CONTROL** | 32-bit | Control register — selects active stack pointer (MSP vs PSP), privileged vs unprivileged mode |

### xPSR Bit Layout

```
 31          28 27       20 19       16 15    10 9     0
┌──────────────┬───────────┬───────────┬────────┬────────┐
│ N Z C V Q    │   IT      │   T       │  ISR  │  (reserved) │
│ (APSR)       │ (EPSR)    │ (EPSR)    │ (IPSR)│          │
└──────────────┴───────────┴───────────┴────────┴────────┘
```

**Key flags:**
- **N (bit 31)** — Negative flag: set when result is negative (MSB = 1)
- **Z (bit 30)** — Zero flag: set when result is zero
- **C (bit 29)** — Carry flag: set on unsigned overflow
- **V (bit 28)** — Overflow flag: set on signed overflow
- **ISR (bits 0–8)** — Exception number of current ISR (0 = thread mode)

## Implementation

### Data Structure (`cpu_state.h`)

```c
typedef struct {
    uint32_t regs[16];     // R0-R15 (R13=SP, R14=LR, R15=PC)
    uint32_t xpsr;         // Program Status Register
    uint32_t primask;      // Priority mask
    uint32_t faultmask;    // Fault mask
    uint32_t basepri;      // Base priority mask
    uint32_t control;      // Control register
    uint32_t msp;          // Main stack pointer
    uint32_t psp;          // Process stack pointer
    uint32_t pc;           // Program counter (alias for regs[15])
} CPU_State;
```

### Reset Logic (`cpu_state.c`)

```c
void cpu_reset(CPU_State *cpu) {
    for (int i = 0; i < 16; i++) cpu->regs[i] = 0;
    cpu->xpsr = 0;
    cpu->primask = 0;
    cpu->faultmask = 0;
    cpu->basepri = 0;
    cpu->control = 0;
    cpu->msp = 0;
    cpu->psp = 0;
    cpu->pc = 0;
}
```

**Note:** In a full implementation, the initial MSP and PC would be loaded from the vector table at `0x08000000` (initial MSP) and `0x08000004` (reset handler address). The current implementation sets them to zero, relying on the simulator's `main.c` to set `PC = FLASH_BASE_ADDR`.

## Interaction with Other Components

| Component | Interaction | Description |
|-----------|-------------|-------------|
| [[instruction_execution\|Instruction Executor]] | Reads/writes registers | Every instruction reads source registers and writes destination registers |
| [[memory_model\|Memory]] | PC → memory read | Fetch phase reads instruction from Flash at `PC` |
| [[interrupt_controller\|NVIC]] | PRIMASK, xPSR.IPSR | NVIC reads PRIMASK to determine if interrupts are masked; writes ISR number to xPSR on exception entry |
| [[memory_model\|Stack Operations]] | SP (R13) → SRAM | PUSH/POP, BL/BX use SP for stack operations |

## Execution Model

The CPU operates in **Thumb mode** (16-bit instructions), which is the only mode supported by Cortex-M3. Key characteristics:

- **Little-endian** byte ordering
- **Word-aligned** access for 32-bit operations
- **PC-relative** addressing for literal pools (PC is aligned to 4-byte boundary during LDR)
- **Automatic PC increment** by 2 after each instruction fetch

## Current Limitations

| Limitation | Impact | Future Work |
|------------|--------|-------------|
| No exception entry/exit modeling | Interrupts don't actually change CPU state | Implement stacking/unstacking, EXC_RETURN |
| No dual stack pointer switching | CONTROL[1] (SPSEL) is ignored | Implement MSP/PSP switching |
| No privilege levels | CONTROL[0] is ignored | Implement privileged vs unprivileged modes |
| No fault handling | HardFault, UsageFault not modeled | Add fault exception routing |

---

#cpu #cortex-m3 #registers #arm-architecture #core
