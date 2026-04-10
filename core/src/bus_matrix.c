#include "bus_matrix.h"
#include <stdio.h>
#include <string.h>

void bus_matrix_init(Bus_Matrix_State *bus) {
    // Инициализация базовых адресов шин
    bus->ahb_base = AHB_BASE_ADDR;
    bus->apb1_base = APB1_BASE_ADDR;
    bus->apb2_base = APB2_BASE_ADDR;
    
    // Инициализация регистров RCC
    bus->rcc_cr = 0;
    bus->rcc_cfgr = 0;
    bus->rcc_apb2rstr = 0;
    bus->rcc_apb1rstr = 0;
    bus->rcc_ahbenr = 0;
    bus->rcc_apb2enr = 0;
    bus->rcc_apb1enr = 0;
    
    // Инициализация DMA
    bus->dma_isr = 0;
    bus->dma_ifcr = 0;
    
    // Инициализация каналов DMA
    for (int i = 0; i < 7; i++) {
        bus->dma_channels[i].ccr = 0;
        bus->dma_channels[i].cndtr = 0;
        bus->dma_channels[i].cpar = 0;
        bus->dma_channels[i].cmar = 0;
    }
}

uint32_t bus_read(Bus_Matrix_State *bus, uint32_t addr) {
    // Проверка доступности адреса
    if (!bus_is_accessible(bus, addr)) {
        //printf("[BUS] Attempted to read from inaccessible address 0x%08X\n", addr);
        return 0xFFFFFFFF; // Возвращаем ошибку
    }
    
    // Обработка разных типов адресов
    if (addr >= RCC_BASE_ADDR && addr < RCC_BASE_ADDR + 0x30) {
        // RCC регистры
        uint32_t offset = addr - RCC_BASE_ADDR;
        switch (offset) {
            case RCC_CR_OFFSET:
                return bus->rcc_cr;
            case RCC_CFGR_OFFSET:
                return bus->rcc_cfgr;
            case RCC_APB2RSTR_OFFSET:
                return bus->rcc_apb2rstr;
            case RCC_APB1RSTR_OFFSET:
                return bus->rcc_apb1rstr;
            case RCC_AHBENR_OFFSET:
                return bus->rcc_ahbenr;
            case RCC_APB2ENR_OFFSET:
                return bus->rcc_apb2enr;
            case RCC_APB1ENR_OFFSET:
                return bus->rcc_apb1enr;
            default:
                //printf("[RCC] Reading from unknown register 0x%08X\n", addr);
                return 0;
        }
    }
    else if (addr >= DMA1_BASE_ADDR && addr < DMA1_BASE_ADDR + 0x80) {
        // DMA регистры
        uint32_t offset = addr - DMA1_BASE_ADDR;
        if (offset >= DMA_ISR_OFFSET && offset < DMA_ISR_OFFSET + 0x10) {
            // Обработка ISR регистра
            return bus->dma_isr;
        }
        else if (offset >= DMA_IFCR_OFFSET && offset < DMA_IFCR_OFFSET + 0x10) {
            // Обработка IFCR регистра
            return bus->dma_ifcr;
        }
        else if (offset >= DMA_CCR1_OFFSET && offset < DMA_CCR1_OFFSET + 0x20) {
            // Обработка регистра канала DMA1
            int channel = (offset - DMA_CCR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
            if (channel >= 0 && channel < 7) {
                return bus->dma_channels[channel].ccr;
            }
        }
        //printf("[DMA] Reading from unknown DMA register 0x%08X\n", addr);
        return 0;
    }
    else if (addr >= GPIOA_BASE_ADDR && addr < GPIOA_BASE_ADDR + 0x400) {
        // GPIO регистры - для простоты возвращаем 0
        // В реальной реализации здесь должна быть логика обращения к GPIO модулю
        return 0;
    }
    else if (addr >= TIM6_BASE_ADDR && addr < TIM6_BASE_ADDR + 0x30) {
        // TIM6 регистры - для простоты возвращаем 0
        // В реальной реализации здесь должна быть логика обращения к TIM6 модулю
        return 0;
    }
    
    // По умолчанию возвращаем 0
    return 0;
}

void bus_write(Bus_Matrix_State *bus, uint32_t addr, uint32_t value) {
    // Проверка доступности адреса
    if (!bus_is_accessible(bus, addr)) {
        //printf("[BUS] Attempted to write to inaccessible address 0x%08X\n", addr);
        return;
    }
    
    // Обработка разных типов адресов
    if (addr >= RCC_BASE_ADDR && addr < RCC_BASE_ADDR + 0x30) {
        // RCC регистры
        uint32_t offset = addr - RCC_BASE_ADDR;
        switch (offset) {
            case RCC_CR_OFFSET:
                bus->rcc_cr = value;
                break;
            case RCC_CFGR_OFFSET:
                bus->rcc_cfgr = value;
                break;
            case RCC_APB2RSTR_OFFSET:
                bus->rcc_apb2rstr = value;
                break;
            case RCC_APB1RSTR_OFFSET:
                bus->rcc_apb1rstr = value;
                break;
            case RCC_AHBENR_OFFSET:
                bus->rcc_ahbenr = value;
                break;
            case RCC_APB2ENR_OFFSET:
                bus->rcc_apb2enr = value;
                break;
            case RCC_APB1ENR_OFFSET:
                bus->rcc_apb1enr = value;
                break;
            default:
                //printf("[RCC] Writing to unknown register 0x%08X\n", addr);
                break;
        }
    }
    else if (addr >= DMA1_BASE_ADDR && addr < DMA1_BASE_ADDR + 0x80) {
        // DMA регистры
        uint32_t offset = addr - DMA1_BASE_ADDR;
        if (offset >= DMA_ISR_OFFSET && offset < DMA_ISR_OFFSET + 0x10) {
            // Обработка ISR регистра
            bus->dma_isr = value;
        }
        else if (offset >= DMA_IFCR_OFFSET && offset < DMA_IFCR_OFFSET + 0x10) {
            // Обработка IFCR регистра
            bus->dma_ifcr = value;
        }
        else if (offset >= DMA_CCR1_OFFSET && offset < DMA_CCR1_OFFSET + 0x20) {
            // Обработка регистра канала DMA1
            int channel = (offset - DMA_CCR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
            if (channel >= 0 && channel < 7) {
                bus->dma_channels[channel].ccr = value;
            }
        }
        //printf("[DMA] Writing to unknown DMA register 0x%08X\n", addr);
    }
    else if (addr >= GPIOA_BASE_ADDR && addr < GPIOA_BASE_ADDR + 0x400) {
        // GPIO регистры - для простоты игнорируем
        // В реальной реализации здесь должна быть логика обращения к GPIO модулю
    }
    else if (addr >= TIM6_BASE_ADDR && addr < TIM6_BASE_ADDR + 0x30) {
        // TIM6 регистры - для простоты игнорируем
        // В реальной реализации здесь должна быть логика обращения к TIM6 модулю
    }
}

uint8_t bus_is_accessible(Bus_Matrix_State *bus, uint32_t addr) {
    // Проверка, доступен ли адрес через шину
    if (addr >= FLASH_BASE_ADDR && addr < FLASH_BASE_ADDR + FLASH_SIZE) {
        return 1; // Flash доступен
    }
    if (addr >= SRAM_BASE_ADDR && addr < SRAM_BASE_ADDR + SRAM_SIZE) {
        return 1; // SRAM доступен
    }
    if (addr >= AHB_BASE_ADDR && addr < AHB_BASE_ADDR + AHB_SIZE) {
        return 1; // AHB шина доступна
    }
    if (addr >= APB1_BASE_ADDR && addr < APB1_BASE_ADDR + APB1_SIZE) {
        return 1; // APB1 шина доступна
    }
    if (addr >= APB2_BASE_ADDR && addr < APB2_BASE_ADDR + APB2_SIZE) {
        return 1; // APB2 шина доступна
    }
    
    return 0; // Неизвестный адрес
}