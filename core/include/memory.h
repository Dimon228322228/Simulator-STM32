#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include "memory_map.h"

typedef struct {
    uint8_t *flash;
    uint8_t *sram;
} Memory;

// Инициализация памяти (выделение массивов)
bool memory_init(Memory *mem);

// Освобождение памяти
void memory_free(Memory *mem);

// Чтение байта (для будущего fetch)
uint8_t memory_read_byte(Memory *mem, uint32_t addr);

// Запись байта (для будущего выполнения инструкций)
void memory_write_byte(Memory *mem, uint32_t addr, uint8_t value);

// Запись полуслова (16 бит)
void memory_write_halfword(Memory *mem, uint32_t addr, uint16_t value);

// Запись слова (32 бит)
void memory_write_word(Memory *mem, uint32_t addr, uint32_t value);

// Secure functions with boundary checking
bool memory_write_halfword_safe(Memory *mem, uint32_t addr, uint16_t value);
bool memory_write_word_safe(Memory *mem, uint32_t addr, uint32_t value);

uint16_t memory_read_halfword(Memory *mem, uint32_t addr);

// Чтение 32-битного слова (для периферии)
uint32_t memory_read_word(Memory *mem, uint32_t addr);

#endif // MEMORY_H