/**
 * memory.c - Flash and SRAM memory subsystem
 *
 * Implements the STM32F103C8T6 memory map:
 *   Flash: 0x08000000 – 0x0800FFFF  (64 KB, read-only during execution)
 *   SRAM:  0x20000000 – 0x20004FFF  (20 KB, read/write)
 *
 * All accesses are little-endian (Cortex-M3 native).
 */

#include "memory.h"
#include <stdlib.h>
#include <stdio.h>

bool memory_init(Memory *mem) {
    mem->flash = (uint8_t *)calloc(FLASH_SIZE, sizeof(uint8_t));
    if (!mem->flash) {
        perror("Failed to allocate Flash memory");
        return false;
    }

    mem->sram = (uint8_t *)calloc(SRAM_SIZE, sizeof(uint8_t));
    if (!mem->sram) {
        perror("Failed to allocate SRAM memory");
        free(mem->flash);
        return false;
    }

    return true;
}

void memory_free(Memory *mem) {
    free(mem->flash);
    free(mem->sram);
    mem->flash = NULL;
    mem->sram  = NULL;
}

/* ------------------------------------------------------------------ */
/*  Byte access                                                         */
/* ------------------------------------------------------------------ */

uint8_t memory_read_byte(Memory *mem, uint32_t addr) {
    if (addr >= FLASH_BASE_ADDR && addr < FLASH_BASE_ADDR + FLASH_SIZE) {
        return mem->flash[addr - FLASH_BASE_ADDR];
    }
    if (addr >= SRAM_BASE_ADDR && addr < SRAM_BASE_ADDR + SRAM_SIZE) {
        return mem->sram[addr - SRAM_BASE_ADDR];
    }
    /* Peripheral region – not routed in this module */
    return 0xFFU;
}

void memory_write_byte(Memory *mem, uint32_t addr, uint8_t value) {
    if (addr >= SRAM_BASE_ADDR && addr < SRAM_BASE_ADDR + SRAM_SIZE) {
        mem->sram[addr - SRAM_BASE_ADDR] = value;
        return;
    }
    /* Flash writes are ignored (programming requires a separate mechanism) */
}

/* ------------------------------------------------------------------ */
/*  Multi-byte access (little-endian)                                   */
/* ------------------------------------------------------------------ */

void memory_write_halfword(Memory *mem, uint32_t addr, uint16_t value) {
    memory_write_byte(mem, addr,     value & 0xFFU);
    memory_write_byte(mem, addr + 1, (value >> 8) & 0xFFU);
}

void memory_write_word(Memory *mem, uint32_t addr, uint32_t value) {
    memory_write_byte(mem, addr,     value & 0xFFU);
    memory_write_byte(mem, addr + 1, (value >> 8) & 0xFFU);
    memory_write_byte(mem, addr + 2, (value >> 16) & 0xFFU);
    memory_write_byte(mem, addr + 3, (value >> 24) & 0xFFU);
}

uint16_t memory_read_halfword(Memory *mem, uint32_t addr) {
    uint8_t low  = memory_read_byte(mem, addr);
    uint8_t high = memory_read_byte(mem, addr + 1);
    return (uint16_t)(low | (high << 8));
}

uint32_t memory_read_word(Memory *mem, uint32_t addr) {
    uint8_t b0 = memory_read_byte(mem, addr);
    uint8_t b1 = memory_read_byte(mem, addr + 1);
    uint8_t b2 = memory_read_byte(mem, addr + 2);
    uint8_t b3 = memory_read_byte(mem, addr + 3);
    return (uint32_t)b0 | ((uint32_t)b1 << 8) |
           ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}
