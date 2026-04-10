#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include "gpio.h"

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
    // В будущем здесь будет периферия
    // Для GPIO регистров возвращаем 0xFF как ошибку доступа
    return 0xFF; // Ошибка доступа
}

void memory_write_byte(Memory *mem, uint32_t addr, uint8_t value) {
    if (addr >= SRAM_BASE_ADDR && addr < (SRAM_BASE_ADDR + SRAM_SIZE)) {
        mem->sram[addr - SRAM_BASE_ADDR] = value;
        return;
    }
    // Flash запись обычно игнорируется при выполнении (нужна отдельная логика прошивки)
}

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
    // Cortex-M3 Little Endian. Младший байт по младшему адресу.
    uint8_t low = memory_read_byte(mem, addr);
    uint8_t high = memory_read_byte(mem, addr + 1);
    return (uint16_t)(low | (high << 8));
}

// Функция для чтения 32-битного регистра периферии
uint32_t memory_read_word(Memory *mem, uint32_t addr) {
    // Проверяем, является ли адрес GPIO регистром
    // Для простоты реализации, если адрес находится в диапазоне периферии,
    // то предполагаем, что это GPIO регистр
    if (addr >= 0x40000000 && addr < 0x40013000) {
        // В будущем здесь будет реальная обработка GPIO
        // Сейчас возвращаем 0 для тестирования
        return 0;
    }
    
    // Если не GPIO, читаем как обычную память
    uint8_t bytes[4];
    bytes[0] = memory_read_byte(mem, addr);
    bytes[1] = memory_read_byte(mem, addr + 1);
    bytes[2] = memory_read_byte(mem, addr + 2);
    bytes[3] = memory_read_byte(mem, addr + 3);
    
    return (bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
}