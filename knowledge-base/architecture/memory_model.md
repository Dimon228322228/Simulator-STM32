# Memory Model

> [!summary]
> The memory subsystem emulates the STM32F103C8T6 memory map, providing Flash (64 KB) and SRAM (20 KB) regions with byte/halfword/word access in little-endian format.

## Responsibility

The memory subsystem is responsible for:
- **Flash memory emulation** — read-only storage for firmware code and constant data
- **SRAM emulation** — read/write memory for variables, stack, and heap
- **Address decoding** — routing memory accesses to the correct region
- **Multi-byte access** — little-endian byte ordering for halfword (16-bit) and word (32-bit) operations

## Memory Map

```
┌─────────────────────────────────────────┐
│         Code / Flash (64 KB)             │  ← Firmware loaded here
│   0x0800 0000 – 0x0800 FFFF             │
├─────────────────────────────────────────┤
│         SRAM (20 KB)                     │  ← Variables, stack, heap
│   0x2000 0000 – 0x2000 4FFF             │
├─────────────────────────────────────────┤
│         Peripheral Region                │  ← GPIO, TIM6, UART, etc.
│   0x4000 0000 – 0x4001 FFFF             │     (routed to peripheral modules)
├─────────────────────────────────────────┤
│         NVIC / System                    │  ← Interrupt controller
│   0xE000 E000 – ...                     │
└─────────────────────────────────────────┘
```

## Implementation

### Data Structure (`memory.h`)

```c
typedef struct {
    uint8_t *flash;  // Dynamically allocated, 64 KB
    uint8_t *sram;   // Dynamically allocated, 20 KB
} Memory;
```

### Initialization (`memory.c`)

```c
bool memory_init(Memory *mem) {
    mem->flash = (uint8_t *)calloc(FLASH_SIZE, sizeof(uint8_t));  // 64 KB zeroed
    if (!mem->flash) return false;

    mem->sram = (uint8_t *)calloc(SRAM_SIZE, sizeof(uint8_t));   // 20 KB zeroed
    if (!mem->sram) {
        free(mem->flash);
        return false;
    }
    return true;
}
```

Both regions are allocated as **contiguous byte arrays** using `calloc`, ensuring they start zeroed.

### Access Functions

| Function | Size | Direction | Region |
|----------|------|-----------|--------|
| `memory_read_byte` | 8-bit | Read | Flash, SRAM |
| `memory_write_byte` | 8-bit | Write | SRAM only (Flash writes ignored) |
| `memory_read_halfword` | 16-bit | Read | Flash, SRAM |
| `memory_write_halfword` | 16-bit | Write | SRAM only |
| `memory_read_word` | 32-bit | Read | Flash, SRAM |
| `memory_write_word` | 32-bit | Write | SRAM only |

### Little-Endian Byte Ordering

```c
uint32_t memory_read_word(Memory *mem, uint32_t addr) {
    uint8_t b0 = memory_read_byte(mem, addr);      // LSB
    uint8_t b1 = memory_read_byte(mem, addr + 1);
    uint8_t b2 = memory_read_byte(mem, addr + 2);
    uint8_t b3 = memory_read_byte(mem, addr + 3);  // MSB
    return (uint32_t)b0 | ((uint32_t)b1 << 8) |
           ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}
```

This matches the Cortex-M3 native **little-endian** format: the least significant byte is at the lowest address.

### Address Decoding Logic

```c
uint8_t memory_read_byte(Memory *mem, uint32_t addr) {
    if (addr >= FLASH_BASE_ADDR && addr < FLASH_BASE_ADDR + FLASH_SIZE) {
        return mem->flash[addr - FLASH_BASE_ADDR];  // Offset into Flash
    }
    if (addr >= SRAM_BASE_ADDR && addr < SRAM_BASE_ADDR + SRAM_SIZE) {
        return mem->sram[addr - SRAM_BASE_ADDR];    // Offset into SRAM
    }
    return 0xFFU;  // Peripheral/invalid region — returns default value
}
```

**Design decision:** Peripheral accesses (`0x40000000+`) are not handled by the memory module. Instead, the [[data_flow\|bus matrix]] routes them to the appropriate peripheral module ([[gpio\|GPIO]], [[timer\|TIM6]], [[uart\|UART]], etc.).

## Flash vs SRAM Characteristics

| Property | Flash | SRAM |
|----------|-------|------|
| **Size** | 64 KB | 20 KB |
| **Base address** | `0x08000000` | `0x20000000` |
| **Read** | ✅ Yes | ✅ Yes |
| **Write (during execution)** | ❌ No (load-time only) | ✅ Yes |
| **Purpose** | Code, constant data | Variables, stack, heap |
| **Persistence** | Survives reset | Lost on reset |

## Current Limitations

| Limitation | Impact |
|------------|--------|
| No peripheral memory routing in `memory.c` | Peripheral accesses return `0xFF` instead of delegating to peripheral modules |
| No memory protection | No MPU (Memory Protection Unit) emulation |
| No aliasing | Cortex-M3 has aliased regions (e.g., Flash aliased to `0x00000000`) — not implemented |
| No bit-banding | ARM bit-banding region (`0x22000000`) not implemented |

## Interaction with Other Components

| Component | Interaction |
|-----------|-------------|
| [[cpu_core\|CPU Core]] | Fetch phase reads instructions from Flash via `memory_read_halfword` |
| [[instruction_execution\|Instruction Executor]] | LDR/STR instructions read/write SRAM via memory access functions |
| [[microservice\|Orchestrator]] | Loads `.bin` files into Flash via `load_binary_file` in `main.c` |

---

#memory #flash #sram #memory-map #little-endian
