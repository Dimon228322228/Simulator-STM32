#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stdint.h>
#include <stddef.h>

// Адреса и размеры согласно ТЗ п. 4.1.1
#define FLASH_BASE_ADDR 0x08000000U
#define FLASH_SIZE      (64 * 1024) // 64 KB

#define SRAM_BASE_ADDR  0x20000000U
#define SRAM_SIZE       (20 * 1024) // 20 KB

// Периферия (диапазоны для будущего использования)
#define PERIPH_BASE_ADDR 0x40000000U

#endif // MEMORY_MAP_H