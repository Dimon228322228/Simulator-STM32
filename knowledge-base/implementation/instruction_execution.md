# Instruction Execution

> [!summary]
> The instruction executor in `execute.c` implements a Thumb-16 decode-execute loop, supporting data processing, memory access, and branch instructions according to the ARMv7-M architecture.

## Responsibility

The executor is responsible for:
- **Instruction decoding** — extracting opcode and operands from 16-bit Thumb instructions
- **ALU operations** — performing arithmetic and logical operations on CPU registers
- **Memory access** — loading/storing data between registers and memory
- **Branching** — changing the Program Counter (PC) for conditional and unconditional jumps
- **Flag management** — updating xPSR flags (N, Z, C, V) after operations

## Execution Loop

```c
void simulator_run(Simulator *sim, int max_steps) {
    int steps = 0;
    while (sim->cpu.pc >= FLASH_BASE_ADDR &&
           sim->cpu.pc < FLASH_BASE_ADDR + FLASH_SIZE &&
           sim->cpu.pc != 0xFFFFFFFFU &&
           steps < max_steps) {
        simulator_step(sim);
        steps++;
    }
}
```

**Termination conditions:**
1. PC leaves Flash region → invalid code address
2. PC reaches `0xFFFFFFFF` → halt instruction
3. Step limit reached → prevent infinite loops

## Instruction Decoder

Each `simulator_step()` call performs:

```c
void simulator_step(Simulator *sim) {
    // 1. FETCH
    uint16_t instr = memory_read_halfword(mem, cpu->pc);
    cpu->pc += 2;  // Thumb PC always advances by 2
    
    // 2. DECODE
    uint16_t opcode = get_bits(instr, 11, 15);
    
    // 3. EXECUTE
    switch (opcode) {
        // ... case handlers ...
    }
}
```

## Supported Instructions

### Data Processing — Immediate

| Opcode | Instruction | Format | Description |
|--------|------------|--------|-------------|
| `0b00000` | **ADDS** Rd, Rn, #imm3 | `00000 imm3 Rn Rd` | Add 3-bit immediate |
| `0b00001` | **SUBS** Rd, Rn, #imm3 | `00001 imm3 Rn Rd` | Subtract 3-bit immediate |
| `0b00010` | **ADDS** Rd, #imm8 | `00010 Rd imm8` | Add 8-bit immediate to Rd |
| `0b00100` | **MOVS** Rd, #imm8 | `00100 Rd imm8` | Load 8-bit immediate into Rd |
| `0b00101` | **SUBS** Rd, #imm8 | `00101 Rd imm8` | Subtract 8-bit immediate from Rd |
| `0b00110` | **ADDS** Rd, Rn, #imm8 | `00110 imm8 Rn Rd` | Add 8-bit immediate |
| `0b00111` | **SUBS** Rd, Rn, #imm8 | `00111 imm8 Rn Rd` | Subtract 8-bit immediate |

### Data Processing — Register

| Opcode | Instruction | Format | Description |
|--------|------------|--------|-------------|
| `0b00011` | **ADDS/SUBS/CMP/MOVS** | `00011 op Rm Rn Rd` | Multi-op group (see below) |

The `0b00011` group uses bits [10:9] to select the operation:

| op | Instruction | Operation |
|----|------------|-----------|
| `00` | **ADDS** Rd, Rn, Rm | Rd = Rn + Rm |
| `01` | **SUBS** Rd, Rn, Rm | Rd = Rn - Rm |
| `10` | **CMP** Rn, Rm | Rn - Rm (flags only, no write) |
| `11` | **MOVS** Rd, Rm | Rd = Rm |

### Extended Data Processing

| Opcode | Instruction | Description |
|--------|------------|-------------|
| `0b01000` | **AND/EOR/LSL/NEGS** | Multi-op group (bits [9:8]) |
| `0b01001` | **ADC/SBC/ROR/MULS** | Multi-op group (bits [9:8]) |

**Opcode `0b01000` sub-operations:**

| op | Instruction | Operation |
|----|------------|-----------|
| `00` | **ANDS** Rd, Rm | Rd = Rd & Rm |
| `01` | **EORS** Rd, Rm | Rd = Rd ^ Rm |
| `10` | **LSLS** Rd, Rm | Rd = Rd << (Rm & 0x1F) |
| `11` | **NEGS** Rd, Rm | Rd = 0 - Rm |

**Opcode `0b01001` sub-operations:**

| op | Instruction | Operation |
|----|------------|-----------|
| `00` | **ADCS** Rd, Rn, Rm | Rd = Rn + Rm + C |
| `01` | **SBCS** Rd, Rn, Rm | Rd = Rn - Rm - !C |
| `10` | **RORS** Rd, Rn, Rm | Rd = Rn >> (Rm & 0x1F) rotated |
| `11` | **MULS** Rd, Rn, Rm | Rd = Rn * Rm |

### Load/Store — Register Offset

| Opcode | Instruction | Description |
|--------|------------|-------------|
| `0b01010` | **STR** Rd, [Rn, Rm] | Store word |
| `0b01011` | **STRH** Rd, [Rn, Rm] | Store halfword |
| `0b01100` | **STRB** Rd, [Rn, Rm] | Store byte |
| `0b01101` | **LDR** Rd, [Rn, Rm] | Load word |
| `0b01110` | **LDRB** Rd, [Rn, Rm] | Load byte (zero-extended) |
| `0b01111` | **LDRH** Rd, [Rn, Rm] | Load halfword (zero-extended) |

### Load/Store — Immediate Offset

| Opcode | Instruction | Description |
|--------|------------|-------------|
| `0b10000` | **STRB** Rd, [Rn, #imm5] | Store byte with 5-bit offset |
| `0b10001` | **LDRB** Rd, [Rn, #imm5] | Load byte with 5-bit offset |
| `0b10010` | **STRH** Rd, [Rn, #imm5] | Store halfword (offset × 2) |
| `0b10011` | **LDRH** Rd, [Rn, #imm5] | Load halfword (offset × 2) |
| `0b10100` | **STR** Rd, [SP, #imm8] | Store to stack (offset × 4) |
| `0b10101` | **LDR** Rd, [SP, #imm8] | Load from stack (offset × 4) |
| `0b11000` | **STR** Rd, [Rn, #imm5] | Store word (offset × 4) |
| `0b11001` | **LDR** Rd, [Rn, #imm5] | Load word (offset × 4) |

### PC/SP Relative

| Opcode | Instruction | Description |
|--------|------------|-------------|
| `0b10110` | **LDR** Rd, [PC, #imm8] | Load from literal pool (offset × 4) |
| `0b10111` | **ADD** Rd, SP, #imm8 | Add to stack pointer (offset × 4) |

### Branch Instructions

| Opcode | Instruction | Description |
|--------|------------|-------------|
| `0b11010/0b11011` | **B.cond** | Conditional branch (8-bit signed offset × 2) |
| `0b11100` | **B** label | Unconditional branch (11-bit signed offset × 2) |
| `0b11111` | **BX** Rm | Branch indirect (PC = Rm & ~1) |
| `0b11101/0b11110` | **BL/BLX** | 32-bit instruction (stub — not implemented) |

### Condition Codes (for B.cond)

| Cond | Suffix | Meaning | Condition |
|------|--------|---------|-----------|
| `0000` | EQ | Equal | Z = 1 |
| `0001` | NE | Not equal | Z = 0 |
| `0010` | CS/HS | Carry set / Unsigned ≥ | C = 1 |
| `0011` | CC/LO | Carry clear / Unsigned < | C = 0 |
| `0100` | MI | Minus / Negative | N = 1 |
| `0101` | PL | Plus / Positive or zero | N = 0 |
| `0110` | VS | Overflow set | V = 1 |
| `0111` | VC | Overflow clear | V = 0 |
| `1000` | HI | Unsigned higher | C = 1 && Z = 0 |
| `1001` | LS | Unsigned lower or same | C = 0 \|\| Z = 1 |
| `1010` | GE | Signed ≥ | N = V |
| `1011` | LT | Signed < | N ≠ V |
| `1100` | GT | Signed > | Z = 0 && N = V |
| `1101` | LE | Signed ≤ | Z = 1 \|\| N ≠ V |
| `1110` | AL | Always | 1 |
| `1111` | — | **SVC** | Supervisor call (halts) |

## Flag Update Logic

### N and Z Flags

```c
static void update_flags(CPU_State *cpu, uint32_t result) {
    // N flag: set if result is negative (bit 31 = 1)
    cpu->xpsr = (cpu->xpsr & ~0x80000000U) | 
                (result & 0x80000000U ? 0x80000000U : 0);
    
    // Z flag: set if result is zero
    cpu->xpsr = (cpu->xpsr & ~0x40000000U) | 
                (result == 0 ? 0x40000000U : 0);
}
```

### Arithmetic Flags (N, Z, C, V)

```c
static void update_arithmetic_flags(CPU_State *cpu, uint32_t a, uint32_t b,
                                     uint32_t result, uint8_t is_subtract) {
    // N: negative
    cpu->xpsr = (cpu->xpsr & ~0x80000000U) | 
                (result & 0x80000000U ? 0x80000000U : 0);
    
    // Z: zero
    cpu->xpsr = (cpu->xpsr & ~0x40000000U) | 
                (result == 0 ? 0x40000000U : 0);
    
    // C: carry
    if (is_subtract) {
        cpu->xpsr = (cpu->xpsr & ~0x100U) | (a >= b ? 0x100U : 0);
    } else {
        cpu->xpsr = (cpu->xpsr & ~0x100U) | (result < a ? 0x100U : 0);
    }
    
    // V: signed overflow
    uint8_t a_sign = (a & 0x80000000U) ? 1 : 0;
    uint8_t b_sign = (b & 0x80000000U) ? 1 : 0;
    uint8_t r_sign = (result & 0x80000000U) ? 1 : 0;
    cpu->xpsr = (cpu->xpsr & ~0x200U) |
                (((a_sign ^ b_sign) & (a_sign ^ r_sign)) ? 0x200U : 0);
}
```

## Sign Extension for Branch Offsets

```c
static inline int32_t sign_extend(uint32_t value, int bits) {
    uint32_t sign_bit = 1U << (bits - 1);
    if (value & sign_bit) {
        return (int32_t)(value | (~0U << bits));
    }
    return (int32_t)value;
}
```

**Example:** 8-bit offset `0xFF` (255 unsigned) → `-1` signed
```
sign_extend(0xFF, 8):
  sign_bit = 1 << 7 = 0x80
  0xFF & 0x80 → true (negative)
  result = 0xFF | (~0 << 8) = 0xFFFFFFFF = -1
```

## Current Limitations

| Limitation | Impact |
|------------|--------|
| No 32-bit Thumb-2 instructions | BL, BLX, LDRD, STRD not supported |
| No MSR/MRS | Special register access not implemented |
| No NOP/BKPT | No-op and breakpoint instructions not handled |
| No IT block | If-Then block for conditional execution not supported |
| No CBZ/CBNZ | Compare and branch on zero not implemented |
| No PUSH/POP | Stack operations defined but need memory routing |
| Peripheral memory routing missing | STR to GPIO/UART addresses silently dropped |

---

#instructions #thumb-16 #decoder #alu #branching #flags #armv7-m
