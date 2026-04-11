# Memory Access

> [!summary]
> Memory access in the simulator implements the Cortex-M3 load/store architecture with little-endian byte ordering, supporting byte, halfword, and word operations across Flash and SRAM regions.

## Load/Store Architecture

Cortex-M3 uses a **load/store architecture**:
- **Only LDR/STR instructions** can access memory
- **ALU operations** work only on registers
- **No memory-to-memory** operations

## Address Space Layout

```
┌───────────────────────────────────────┐
│ 0xFFFFFFFF                             │
│ ...                                    │
│ 0xE0000000  System peripherals (NVIC)  │
│ ...                                    │
│ 0x40020000  AHB peripherals            │
│ 0x40010000  APB2 peripherals (GPIO)    │
│ 0x40000000  APB1 peripherals (TIM6)    │
│ ...                                    │
│ 0x20004FFF  SRAM end                   │
│ 0x20000000  SRAM (20 KB)               │
│ ...                                    │
│ 0x0800FFFF  Flash end                  │
│ 0x08000000  Flash (64 KB)              │
│ ...                                    │
│ 0x00000000  Boot memory (aliased)      │
└───────────────────────────────────────┘
```

## Access Size and Alignment

| Access Type | Size | Alignment Required | Instructions |
|------------|------|-------------------|--------------|
| **Byte** | 8-bit (1 byte) | None | LDRB, STRB |
| **Halfword** | 16-bit (2 bytes) | 2-byte aligned | LDRH, STRH |
| **Word** | 32-bit (4 bytes) | 4-byte aligned | LDR, STR |

### Unaligned Access

Cortex-M3 **supports unaligned access** for most instructions, but with a performance penalty. The simulator currently assumes **aligned access** for simplicity.

## Byte Access

### Read Byte

```c
uint8_t memory_read_byte(Memory *mem, uint32_t addr) {
    if (addr >= FLASH_BASE_ADDR && addr < FLASH_BASE_ADDR + FLASH_SIZE) {
        return mem->flash[addr - FLASH_BASE_ADDR];
    }
    if (addr >= SRAM_BASE_ADDR && addr < SRAM_BASE_ADDR + SRAM_SIZE) {
        return mem->sram[addr - SRAM_BASE_ADDR];
    }
    return 0xFFU;  // Peripheral/invalid region
}
```

**Address decoding:**
- Flash: `addr - 0x08000000` → index into flash[]
- SRAM: `addr - 0x20000000` → index into sram[]
- Other: return `0xFF` (default value for unmapped regions)

### Write Byte

```c
void memory_write_byte(Memory *mem, uint32_t addr, uint8_t value) {
    if (addr >= SRAM_BASE_ADDR && addr < SRAM_BASE_ADDR + SRAM_SIZE) {
        mem->sram[addr - SRAM_BASE_ADDR] = value;
        return;
    }
    // Flash writes ignored (Flash programming requires special sequence)
    // Peripheral writes not handled here (routed to peripheral modules)
}
```

**Design decision:** Flash is **read-only during execution**. Writing to Flash requires a special programming sequence (not implemented).

## Halfword Access (Little-Endian)

### Read Halfword

```c
uint16_t memory_read_halfword(Memory *mem, uint32_t addr) {
    uint8_t low  = memory_read_byte(mem, addr);
    uint8_t high = memory_read_byte(mem, addr + 1);
    return (uint16_t)(low | (high << 8));
}
```

**Example:** Flash contains `[0x05, 0x20]` at address `0x08000000`
```
memory_read_halfword(mem, 0x08000000):
  low  = flash[0] = 0x05
  high = flash[1] = 0x20
  result = 0x05 | (0x20 << 8) = 0x2005
```

### Write Halfword

```c
void memory_write_halfword(Memory *mem, uint32_t addr, uint16_t value) {
    memory_write_byte(mem, addr,     value & 0xFFU);
    memory_write_byte(mem, addr + 1, (value >> 8) & 0xFFU);
}
```

**Example:** `memory_write_halfword(mem, 0x20000000, 0x1234)`
```
memory_write_byte(mem, 0x20000000, 0x34)  → sram[0] = 0x34
memory_write_byte(mem, 0x20000001, 0x12)  → sram[1] = 0x12
```

## Word Access (Little-Endian)

### Read Word

```c
uint32_t memory_read_word(Memory *mem, uint32_t addr) {
    uint8_t b0 = memory_read_byte(mem, addr);
    uint8_t b1 = memory_read_byte(mem, addr + 1);
    uint8_t b2 = memory_read_byte(mem, addr + 2);
    uint8_t b3 = memory_read_byte(mem, addr + 3);
    return (uint32_t)b0 | ((uint32_t)b1 << 8) |
           ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}
```

### Write Word

```c
void memory_write_word(Memory *mem, uint32_t addr, uint32_t value) {
    memory_write_byte(mem, addr,     value & 0xFFU);
    memory_write_byte(mem, addr + 1, (value >> 8) & 0xFFU);
    memory_write_byte(mem, addr + 2, (value >> 16) & 0xFFU);
    memory_write_byte(mem, addr + 3, (value >> 24) & 0xFFU);
}
```

## LDR Instruction Addressing Modes

### Register Offset: `LDR Rd, [Rn, Rm]`

```c
case 0b01101: {
    uint32_t rm   = get_bits(instr, 6, 8);
    uint32_t rn   = get_bits(instr, 3, 5);
    uint32_t rd   = get_bits(instr, 0, 2);
    uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
    cpu->regs[rd] = memory_read_word(mem, addr);
    break;
}
```

### Immediate Offset: `LDR Rd, [Rn, #imm5]`

```c
case 0b11001: {
    uint32_t rd  = get_bits(instr, 0, 2);
    uint32_t rn  = get_bits(instr, 3, 5);
    uint32_t imm = get_bits(instr, 6, 10);
    cpu->regs[rd] = memory_read_word(mem, cpu->regs[rn] + (imm << 2));
    break;
}
```

**Note:** The offset is **shifted left by 2** (multiplied by 4) because word accesses must be 4-byte aligned.

### PC-Relative: `LDR Rd, [PC, #imm8]`

```c
case 0b10110: {
    uint32_t rd  = get_bits(instr, 8, 10);
    uint32_t imm = get_bits(instr, 0, 7);
    uint32_t addr = (cpu->pc & ~0x3U) + (imm << 2);
    cpu->regs[rd] = memory_read_word(mem, addr);
    break;
}
```

**Key detail:** `PC & ~0x3` aligns PC to 4-byte boundary before adding offset.

### Stack-Relative: `LDR Rd, [SP, #imm8]`

```c
case 0b10101: {
    uint32_t rd  = get_bits(instr, 8, 10);
    uint32_t imm = get_bits(instr, 0, 7);
    cpu->regs[rd] = memory_read_word(mem, cpu->regs[13] + (imm << 2));
    break;
}
```

Uses R13 (SP) as base address, offset shifted by 2.

## Memory Initialization

### Firmware Loading

```c
int load_binary_file(Memory *mem, const char *filename) {
    FILE *file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size > FLASH_SIZE) {
        fprintf(stderr, "[ERROR] File too large\n");
        return 0;
    }
    
    size_t bytes_read = fread(mem->flash, 1, file_size, file);
    fclose(file);
    
    printf("[INIT] Loaded %ld bytes from %s into Flash at 0x%08X\n",
           file_size, filename, FLASH_BASE_ADDR);
    return 1;
}
```

### Demo Program Loading

```c
uint16_t program[] = {
    0x2005,  /* MOV  R0, #5       */
    0x2103,  /* MOV  R1, #3       */
    0x1842,  /* ADDS R2, R0, R1   → R2 = 8 */
    0xE7FF   /* B    .            (infinite loop) */
};

memcpy(sim.mem.flash, program, sizeof(program));
```

## Current Limitations

| Limitation | Impact |
|------------|--------|
| No peripheral routing | Writes to GPIO/UART addresses silently dropped |
| No bit-banding | Atomic bit manipulation via alias region not supported |
| No MPU | No memory protection between regions |
| No unaligned access handling | Unaligned LDR/STR may produce incorrect results |
| No Flash programming model | Cannot simulate Flash write/erase operations |

---

#memory #load-store #little-endian #alignment #flash #sram #lddr #str
