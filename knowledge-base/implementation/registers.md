# CPU Registers

> [!summary]
> Detailed documentation of the ARM Cortex-M3 register file as implemented in the simulator, including general-purpose registers, special registers, and their usage patterns.

## Register File Overview

The Cortex-M3 has **16 general-purpose registers** (R0–R15) plus several **special-purpose registers** for system control.

### General-Purpose Registers (R0–R12)

| Register | Alias | Thumb-16 Access | Typical Usage |
|----------|-------|-----------------|---------------|
| R0 | r0 | ✅ Full (8-bit and 3-bit immediates) | Function argument 1, return value |
| R1 | r1 | ✅ Full | Function argument 2, return value |
| R2 | r2 | ✅ Full | Function argument 3, temporary |
| R3 | r3 | ✅ Full | Function argument 4, temporary |
| R4 | r4 | ⚠️ Limited (register ops only) | Callee-saved local variable |
| R5 | r5 | ⚠️ Limited | Callee-saved local variable |
| R6 | r6 | ⚠️ Limited | Callee-saved local variable |
| R7 | r7 | ⚠️ Limited | Callee-saved local variable, frame pointer |
| R8 | r8 | ❌ No direct Thumb-16 | Additional local variable |
| R9 | r9 | ❌ No direct Thumb-16 | Additional local variable |
| R10 | r10 | ❌ No direct Thumb-16 | Additional local variable |
| R11 | r11 (FP) | ❌ No direct Thumb-16 | Frame pointer |
| R12 | r12 (IP) | ❌ No direct Thumb-16 | Intra-procedure call scratch register |

### Special Registers (R13–R15)

| Register | Alias | Purpose | Notes |
|----------|-------|---------|-------|
| R13 | SP | Stack Pointer | Must be word-aligned. In Thumb-16, only used with offset instructions. |
| R14 | LR | Link Register | Holds return address after BL instruction. Set to EXC_RETURN on exception entry. |
| R15 | PC | Program Counter | Always points to next instruction + 4 (pipeline). In Thumb mode, bit 0 is always 0. |

## Special System Registers

| Register | Width | Purpose |
|----------|-------|---------|
| **xPSR** | 32-bit | Combined Program Status Register (APSR + IPSR + EPSR) |
| **PRIMASK** | 32-bit | Priority mask — disables all configurable-priority exceptions when set |
| **FAULTMASK** | 32-bit | Fault mask — disables all exceptions except NMI |
| **BASEPRI** | 32-bit | Base priority — masks exceptions with priority ≥ BASEPRI |
| **CONTROL** | 32-bit | Control register — thread/handler mode, privileged/unprivileged, MSP/PSP select |

## xPSR Detailed Layout

```
 31    28 27    20 19    16 15  10 9     0
┌───────┬────────┬────────┬──────┬────────┐
│ NZCVQ │   IT   │   T    │ ISR  │        │
│ APSR  │  EPSR  │  EPSR  │ IPSR │(reserved)│
└───────┴────────┴────────┴──────┴────────┘
```

### APSR Bits (Application Program Status Register)

| Bit | Name | Description |
|-----|------|-------------|
| 31 | **N** | Negative — result has bit 31 set |
| 30 | **Z** | Zero — result is zero |
| 29 | **C** | Carry — unsigned overflow |
| 28 | **V** | Overflow — signed overflow |
| 27 | **Q** | Sticky overflow (DSP instructions) |

### IPSR Bits (Interrupt Program Status Register)

| Bits | Name | Description |
|------|------|-------------|
| 8–0 | **ISR** | Exception number (0 = thread mode, 1–15 = system exceptions, 16+ = peripheral IRQs) |

### EPSR Bits (Execution Program Status Register)

| Bits | Name | Description |
|------|------|-------------|
| 27–25 | **ICI/IT** | Interruptible-continuation / If-Then state |
| 24 | **T** | Thumb state (always 1 on Cortex-M) |

## Register Access Patterns in Thumb-16

### High Register Access (R8–R15)

Most Thumb-16 instructions can only access R0–R7. High registers require specific instructions:

| Instruction | Accessible Registers |
|-------------|---------------------|
| MOV Rd, Rm | Rd, Rm ∈ {R0–R7} (mostly) |
| ADD Rd, Rn, Rm | All in {R0–R7} |
| ADD Rd, SP, #imm | Rd ∈ {R0–R7} |
| LDR Rd, [PC, #imm] | Rd ∈ {R0–R7} |
| LDR/STR [SP, #imm] | Rd ∈ {R0–R7} |
| **MOV Rd, Rm** (special encoding) | One of Rd/Rm can be R8–R15 |
| **ADD Rd, SP, #imm** | Rd can be SP or R0–R7 |
| **BX Rm** | Rm can be R0–R15 |

## Stack Pointer (SP = R13)

The Cortex-M3 has **two stack pointers**:

| Stack Pointer | Usage | Selected by |
|--------------|-------|-------------|
| **MSP** (Main SP) | Handler mode (exceptions), thread mode after reset | CONTROL[1] = 0 |
| **PSP** (Process SP) | Thread mode in user applications | CONTROL[1] = 1 |

In the current implementation, both MSP and PSP are maintained but **not actively switched** — all stack operations use R13 directly.

### Stack Operations

Stack grows **downward** (from high to low addresses):

```
High Address
    │
    ├───┐
    │   │ ← Initial SP (e.g., 0x20005000)
    ├───┘
    │
    ▼  (PUSH, STR to [SP, -offset])
    
    │
    ├───┐
    │   │ ← Current SP after pushes
    ├───┘
    │
    ▼  (more pushes)
    
Low Address
```

**PUSH** (conceptual, not yet implemented):
```
SP -= 4
memory_write_word(SP, value)
```

**POP** (conceptual, not yet implemented):
```
value = memory_read_word(SP)
SP += 4
```

## Link Register (LR = R14)

LR holds the **return address** after a subroutine call:

```
BL subroutine    → LR = PC + 2 (address of instruction after BL)
                  → PC = subroutine_address

...

BX LR            → PC = LR (return to caller)
```

On **exception entry**, LR is set to a special **EXC_RETURN** value:

| EXC_RETURN | Meaning |
|------------|---------|
| `0xFFFFFFF1` | Return to Handler mode, using MSP |
| `0xFFFFFFF9` | Return to Thread mode, using MSP |
| `0xFFFFFFFD` | Return to Thread mode, using PSP |

## Program Counter (PC = R15)

The PC in Thumb mode has special behavior:

- **During instruction fetch**: PC points to the **current instruction**
- **During execution**: PC = current instruction address + 4 (pipeline effect)
- **For LDR [PC, #imm]**: PC is aligned to 4-byte boundary: `PC & ~0x3`
- **After branch**: PC = branch_target (no adjustment needed)

### Example: PC-Relative Load

```
Address    Instruction
0x08000000  MOV R0, #5
0x08000002  MOV R1, #3
0x08000004  LDR R2, [PC, #0]   ← PC = 0x08000004 + 4 = 0x08000008
                                    aligned: 0x08000008
0x08000006  B .
0x08000008  0x0000002A         ← Literal pool: R2 = 42
```

## Implementation (`cpu_state.h`)

```c
typedef struct {
    uint32_t regs[16];     // R0-R15
    uint32_t xpsr;         // Program Status Register
    uint32_t primask;
    uint32_t faultmask;
    uint32_t basepri;
    uint32_t control;
    uint32_t msp;          // Main Stack Pointer
    uint32_t psp;          // Process Stack Pointer
    uint32_t pc;           // Program Counter (alias for regs[15])
} CPU_State;
```

## Reset State

After `cpu_reset()`:
- All R0–R15 = 0
- xPSR = 0
- PRIMASK = FAULTMASK = BASEPRI = CONTROL = 0
- MSP = PSP = 0

In a full implementation, the reset handler would:
1. Load initial MSP from `Flash[0x00..0x03]` (vector table entry 0)
2. Load initial PC from `Flash[0x04..0x07]` (vector table entry 1)
3. Set xPSR.T = 1 (Thumb state)

---

#registers #cpu #r0-r15 #stack-pointer #link-register #program-counter #xpsr
