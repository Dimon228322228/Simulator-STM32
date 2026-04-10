#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include "memory_map.h"

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
    if (mem->flash) free(mem->flash);
    if (mem->sram) free(mem->sram);
    mem->flash = NULL;
    mem->sram = NULL;
}

uint8_t memory_read_byte(Memory *mem, uint32_t addr) {
    if (addr >= FLASH_BASE_ADDR && addr < (FLASH_BASE_ADDR + FLASH_SIZE)) {
        return mem->flash[addr - FLASH_BASE_ADDR];
    }
    if (addr >= SRAM_BASE_ADDR && addr < (SRAM_BASE_ADDR + SRAM_SIZE)) {
        return mem->sram[addr - SRAM_BASE_ADDR];
    }
    // In future this will handle peripherals
    // For GPIO registers return 0xFF as access error
    return 0xFF; // Access error
}

void memory_write_byte(Memory *mem, uint32_t addr, uint8_t value) {
    if (addr >= SRAM_BASE_ADDR && addr < (SRAM_BASE_ADDR + SRAM_SIZE)) {
        mem->sram[addr - SRAM_BASE_ADDR] = value;
        return;
    }
    // Flash write is usually ignored during execution (needs separate flashing logic)
}

// Secure function to write halfword with boundary checking
bool memory_write_halfword_safe(Memory *mem, uint32_t addr, uint16_t value) {
    // Check alignment
    if (addr % 2 != 0) {
        return false; // Address not aligned for halfword
    }
    
    // Check bounds for both bytes
    if (addr >= SRAM_BASE_ADDR && (addr + 1) < (SRAM_BASE_ADDR + SRAM_SIZE)) {
        uint32_t offset = addr - SRAM_BASE_ADDR;
        mem->sram[offset] = value & 0xFF;
        mem->sram[offset + 1] = (value >> 8) & 0xFF;
        return true;
    }
    return false; // Out of bounds
}

// Secure function to write word with boundary checking
bool memory_write_word_safe(Memory *mem, uint32_t addr, uint32_t value) {
    // Check alignment
    if (addr % 4 != 0) {
        return false; // Address not aligned for word
    }
    
    // Check bounds for all four bytes
    if (addr >= SRAM_BASE_ADDR && (addr + 3) < (SRAM_BASE_ADDR + SRAM_SIZE)) {
        uint32_t offset = addr - SRAM_BASE_ADDR;
        mem->sram[offset] = value & 0xFF;
        mem->sram[offset + 1] = (value >> 8) & 0xFF;
        mem->sram[offset + 2] = (value >> 16) & 0xFF;
        mem->sram[offset + 3] = (value >> 24) & 0xFF;
        return true;
    }
    return false; // Out of bounds
}

// Legacy functions that maintain the old behavior (for backward compatibility)
void memory_write_halfword(Memory *mem, uint32_t addr, uint16_t value) {
    memory_write_byte(mem, addr, value & 0xFF);
    memory_write_byte(mem, addr + 1, (value >> 8) & 0xFF);
}

void memory_write_word(Memory *mem, uint32_t addr, uint32_t value) {
    memory_write_byte(mem, addr, value & 0xFF);
    memory_write_byte(mem, addr + 1, (value >> 8) & 0xFF);
    memory_write_byte(mem, addr + 2, (value >> 16) & 0xFF);
    memory_write_byte(mem, addr + 3, (value >> 24) & 0xFF);
}

uint16_t memory_read_halfword(Memory *mem, uint32_t addr) {
    // Check bounds for both bytes
    if (addr >= FLASH_BASE_ADDR && (addr + 1) < (FLASH_BASE_ADDR + FLASH_SIZE)) {
        uint8_t low = mem->flash[addr - FLASH_BASE_ADDR];
        uint8_t high = mem->flash[addr - FLASH_BASE_ADDR + 1];
        return (uint16_t)(low | (high << 8));
    }
    if (addr >= SRAM_BASE_ADDR && (addr + 1) < (SRAM_BASE_ADDR + SRAM_SIZE)) {
        uint8_t low = mem->sram[addr - SRAM_BASE_ADDR];
        uint8_t high = mem->sram[addr - SRAM_BASE_ADDR + 1];
        return (uint16_t)(low | (high << 8));
    }
    // For addresses not in flash or sram range, return error
    return 0xFFFF; // Error value
}

// Function to read 32-bit word with boundary checking
uint32_t memory_read_word(Memory *mem, uint32_t addr) {
    // Check bounds for all four bytes
    if (addr >= FLASH_BASE_ADDR && (addr + 3) < (FLASH_BASE_ADDR + FLASH_SIZE)) {
        uint8_t *ptr = mem->flash + (addr - FLASH_BASE_ADDR);
        return (ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24));
    }
    if (addr >= SRAM_BASE_ADDR && (addr + 3) < (SRAM_BASE_ADDR + SRAM_SIZE)) {
        uint8_t *ptr = mem->sram + (addr - SRAM_BASE_ADDR);
        return (ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24));
    }
    
    // Check if address is in peripheral range
    if (addr >= 0x40000000 && addr < 0x40013000) {
        // In future this will handle GPIO and other peripherals
        // Currently return 0 for testing
        return 0;
    }
    
    // Return error value for invalid addresses
    return 0xFFFFFFFF; // Error value
}