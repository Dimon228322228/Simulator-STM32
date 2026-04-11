# Instruction Decoder

> [!summary]
> The instruction decoder extracts opcode and operand fields from 16-bit Thumb instructions, routing execution to the appropriate handler based on the top 5 bits.

## Responsibility

The decoder is responsible for:
- **Bit field extraction** — isolating opcode, register indices, and immediate values from instruction words
- **Instruction group identification** — mapping the 5-bit opcode to the correct execution branch
- **Sign extension** — converting encoded offsets to signed integers for branch targets
- **Condition evaluation** — checking xPSR flags for conditional branches

## Bit Manipulation Helpers

### Extract Bits

```c
static inline uint32_t get_bits(uint32_t value, int start, int end) {
    uint32_t mask = (1U << (end - start + 1)) - 1;
    return (value >> start) & mask;
}
```

**Example:** Extract opcode from `0x2005` (MOV R0, #5)
```
get_bits(0x2005, 11, 15):
  mask = (1 << 5) - 1 = 0x1F
  result = (0x2005 >> 11) & 0x1F
         = 0x10 & 0x1F = 0x04
```

### Sign Extension

```c
static inline int32_t sign_extend(uint32_t value, int bits) {
    uint32_t sign_bit = 1U << (bits - 1);
    if (value & sign_bit) {
        return (int32_t)(value | (~0U << bits));
    }
    return (int32_t)value;
}
```

**Example:** Branch offset `0x7F` (11-bit) → `+127`
```
sign_extend(0x7F, 11):
  sign_bit = 1 << 10 = 0x400
  0x7F & 0x400 → false (positive)
  result = 0x7F = 127
```

**Example:** Branch offset `0x7FF` (11-bit) → `-1`
```
sign_extend(0x7FF, 11):
  sign_bit = 0x400
  0x7FF & 0x400 → true (negative)
  result = 0x7FF | (~0 << 11) = 0xFFFFFFFF = -1
```

## Opcode Map

The Thumb-16 instruction set uses the top 5 bits (15–11) as the primary opcode:

```
┌──────────────────────────────────────────────────────────┐
│ Opcode (bits 15-11) → Instruction Group                  │
├──────┬───────────────────────────────────────────────────┤
│ 00000│ ADDS   Rd, Rn, #imm3                             │
│ 00001│ SUBS   Rd, Rn, #imm3                             │
│ 00010│ ADDS   Rd, #imm8                                 │
│ 00011│ ADDS/SUBS/CMP/MOVS (register group, bits 10-9)   │
│ 00100│ MOVS   Rd, #imm8                                 │
│ 00101│ SUBS   Rd, #imm8                                 │
│ 00110│ ADDS   Rd, Rn, #imm8                             │
│ 00111│ SUBS   Rd, Rn, #imm8                             │
│ 01000│ AND/EOR/LSL/NEGS (bits 9-8)                      │
│ 01001│ ADC/SBC/ROR/MULS (bits 9-8)                      │
│ 01010│ STR    Rd, [Rn, Rm]                              │
│ 01011│ STRH   Rd, [Rn, Rm]                              │
│ 01100│ STRB   Rd, [Rn, Rm]                              │
│ 01101│ LDR    Rd, [Rn, Rm]                              │
│ 01110│ LDRB   Rd, [Rn, Rm]                              │
│ 01111│ LDRH   Rd, [Rn, Rm]                              │
│ 10000│ STRB   Rd, [Rn, #imm5]                           │
│ 10001│ LDRB   Rd, [Rn, #imm5]                           │
│ 10010│ STRH   Rd, [Rn, #imm5]  (offset × 2)             │
│ 10011│ LDRH   Rd, [Rn, #imm5]                           │
│ 10100│ STR    Rd, [SP, #imm8]  (offset × 4)             │
│ 10101│ LDR    Rd, [SP, #imm8]                           │
│ 10110│ LDR    Rd, [PC, #imm8]  (offset × 4)             │
│ 10111│ ADD    Rd, SP, #imm8                             │
│ 11000│ STR    Rd, [Rn, #imm5]  (offset × 4)             │
│ 11001│ LDR    Rd, [Rn, #imm5]                           │
│ 11010│ B.cond (cond 0xxx)                                │
│ 11011│ B.cond (cond 1xxx) / SVC                         │
│ 11100│ B      label  (unconditional)                     │
│ 11101│ BL/BLX first halfword (32-bit) — stub            │
│ 11110│ BL/BLX second halfword (32-bit) — stub           │
│ 11111│ BX     Rm                                        │
└──────┴───────────────────────────────────────────────────┘
```

## Multi-Opcode Groups

Some opcodes encode a **secondary opcode** in lower bits:

### Opcode 0b00011 — Register Data Processing

```
15        11 10  9 8    6 5    3 2    0
┌──────────┬────┬──────┬──────┬──────┐
│  00011   │ op │  Rm  │  Rn  │  Rd  │
└──────────┴────┴──────┴──────┴──────┘
```

| op (bits 10-9) | Instruction | Operation |
|----------------|-------------|-----------|
| 00 | ADDS Rd, Rn, Rm | Rd = Rn + Rm |
| 01 | SUBS Rd, Rn, Rm | Rd = Rn - Rm |
| 10 | CMP Rn, Rm | Rn - Rm (flags only) |
| 11 | MOVS Rd, Rm | Rd = Rm |

### Opcode 0b01000 — Extended ALU

```
15        11 9  8 5    3 2    0
┌──────────┬────┬──────┬──────┐
│  01000   │ op │  Rm  │  Rd  │
└──────────┴────┴──────┴──────┘
```

| op (bits 9-8) | Instruction | Operation |
|---------------|-------------|-----------|
| 00 | ANDS Rd, Rm | Rd = Rd & Rm |
| 01 | EORS Rd, Rm | Rd = Rd ^ Rm |
| 10 | LSLS Rd, Rm | Rd = Rd << (Rm & 0x1F) |
| 11 | NEGS Rd, Rm | Rd = 0 - Rm |

### Opcode 0b01001 — Multiply/Shift

| op (bits 9-8) | Instruction | Operation |
|---------------|-------------|-----------|
| 00 | ADCS Rd, Rn, Rm | Rd = Rn + Rm + C |
| 01 | SBCS Rd, Rn, Rm | Rd = Rn - Rm - !C |
| 10 | RORS Rd, Rn, Rm | Rd = Rn rotate-right (Rm & 0x1F) |
| 11 | MULS Rd, Rn, Rm | Rd = Rn × Rm |

## Conditional Branch Evaluation

```c
static uint8_t check_condition(uint16_t instr, CPU_State *cpu) {
    uint8_t cond = get_bits(instr, 8, 11);
    
    uint8_t n = (cpu->xpsr & 0x80000000U) ? 1 : 0;
    uint8_t z = (cpu->xpsr & 0x40000000U) ? 1 : 0;
    uint8_t c = (cpu->xpsr & 0x100U) ? 1 : 0;
    uint8_t v = (cpu->xpsr & 0x200U) ? 1 : 0;
    
    switch (cond) {
        case 0b0000: return z;             /* EQ */
        case 0b0001: return !z;            /* NE */
        // ... (all 16 conditions)
        case 0b1110: return 1;             /* AL */
        default:     return 0;             /* NV */
    }
}
```

## Decoder Flow Diagram

```
Instruction (16-bit)
      │
      ▼
┌─────────────────┐
│ Extract bits    │
│ [15:11] → opcode│
└────────┬────────┘
         │
    ┌────┴────┐
    │ Switch  │
    │ opcode  │
    └────┬────┘
         │
    ┌────┼────────────────────┐
    │    │                    │
    ▼    ▼                    ▼
0b00100  0b00011           0b11100
 MOVS    ADDS/SUBS          B
         /CMP/MOVS
         │
    ┌────┴────┐
    │Extract  │
    │bits     │
    │[10:9]   │
    └────┬────┘
         │
    ┌────┼────┐
    ▼    ▼    ▼
   00   01   10   11
  ADDS SUBS CMP MOVS
```

## Error Handling

When an unknown instruction is encountered:

```c
default:
    fprintf(stderr, "[EXEC] Unknown instruction 0x%04X at PC 0x%08X – halting\n",
            instr, fetch_pc);
    cpu->pc = 0xFFFFFFFFU;  // Halt address
    break;
```

This sets the PC to `0xFFFFFFFF`, which causes `simulator_run()` to exit its loop.

---

#decoder #bit-extraction #opcode #thumb-16 #instruction-format
