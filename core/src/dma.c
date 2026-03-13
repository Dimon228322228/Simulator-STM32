#include "dma.h"
#include <stdio.h>
#include <string.h>

void dma_init(DMA_State *dma) {
    // Инициализация всех регистров нулями
    memset(dma, 0, sizeof(DMA_State));
    
    // Инициализация каналов
    for (int i = 0; i < 7; i++) {
        dma->channels[i].ccr = 0;
        dma->channels[i].cndtr = 0;
        dma->channels[i].cpar = 0;
        dma->channels[i].cmar = 0;
    }
}

void dma_reset(DMA_State *dma) {
    // Сброс всех регистров
    memset(dma, 0, sizeof(DMA_State));
    
    // Сброс каналов
    for (int i = 0; i < 7; i++) {
        dma->channels[i].ccr = 0;
        dma->channels[i].cndtr = 0;
        dma->channels[i].cpar = 0;
        dma->channels[i].cmar = 0;
    }
}

uint32_t dma_read_register(DMA_State *dma, uint32_t addr) {
    // Вычисление смещения регистра
    uint32_t offset = addr - DMA1_BASE_ADDR;
    
    if (offset >= DMA_ISR_OFFSET && offset < DMA_ISR_OFFSET + 0x10) {
        // ISR регистр
        return dma->isr;
    }
    else if (offset >= DMA_IFCR_OFFSET && offset < DMA_IFCR_OFFSET + 0x10) {
        // IFCR регистр
        return dma->ifcr;
    }
    else if (offset >= DMA_CCR1_OFFSET && offset < DMA_CCR1_OFFSET + 0x20) {
        // Регистры каналов
        int channel = (offset - DMA_CCR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
        if (channel >= 0 && channel < 7) {
            return dma->channels[channel].ccr;
        }
    }
    else if (offset >= DMA_CNDTR1_OFFSET && offset < DMA_CNDTR1_OFFSET + 0x20) {
        // Регистры количества данных
        int channel = (offset - DMA_CNDTR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
        if (channel >= 0 && channel < 7) {
            return dma->channels[channel].cndtr;
        }
    }
    else if (offset >= DMA_CPAR1_OFFSET && offset < DMA_CPAR1_OFFSET + 0x20) {
        // Регистры адреса периферии
        int channel = (offset - DMA_CPAR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
        if (channel >= 0 && channel < 7) {
            return dma->channels[channel].cpar;
        }
    }
    else if (offset >= DMA_CMAR1_OFFSET && offset < DMA_CMAR1_OFFSET + 0x20) {
        // Регистры адреса памяти
        int channel = (offset - DMA_CMAR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
        if (channel >= 0 && channel < 7) {
            return dma->channels[channel].cmar;
        }
    }
    
    //printf("[DMA] Reading from unknown register 0x%08X\n", addr);
    return 0;
}

void dma_write_register(DMA_State *dma, uint32_t addr, uint32_t value) {
    // Вычисление смещения регистра
    uint32_t offset = addr - DMA1_BASE_ADDR;
    
    if (offset >= DMA_ISR_OFFSET && offset < DMA_ISR_OFFSET + 0x10) {
        // ISR регистр - только для чтения
        //printf("[DMA] Attempted to write to read-only register 0x%08X\n", addr);
    }
    else if (offset >= DMA_IFCR_OFFSET && offset < DMA_IFCR_OFFSET + 0x10) {
        // IFCR регистр
        dma->ifcr = value;
    }
    else if (offset >= DMA_CCR1_OFFSET && offset < DMA_CCR1_OFFSET + 0x20) {
        // Регистры каналов
        int channel = (offset - DMA_CCR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
        if (channel >= 0 && channel < 7) {
            dma->channels[channel].ccr = value;
        }
    }
    else if (offset >= DMA_CNDTR1_OFFSET && offset < DMA_CNDTR1_OFFSET + 0x20) {
        // Регистры количества данных
        int channel = (offset - DMA_CNDTR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
        if (channel >= 0 && channel < 7) {
            dma->channels[channel].cndtr = value;
        }
    }
    else if (offset >= DMA_CPAR1_OFFSET && offset < DMA_CPAR1_OFFSET + 0x20) {
        // Регистры адреса периферии
        int channel = (offset - DMA_CPAR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
        if (channel >= 0 && channel < 7) {
            dma->channels[channel].cpar = value;
        }
    }
    else if (offset >= DMA_CMAR1_OFFSET && offset < DMA_CMAR1_OFFSET + 0x20) {
        // Регистры адреса памяти
        int channel = (offset - DMA_CMAR1_OFFSET) / 0x14; // Каждый канал занимает 0x14 байт
        if (channel >= 0 && channel < 7) {
            dma->channels[channel].cmar = value;
        }
    }
    else {
        //printf("[DMA] Writing to unknown register 0x%08X\n", addr);
    }
}

void dma_start_transfer(DMA_State *dma, int channel) {
    if (channel >= 0 && channel < 7) {
        // Установка флага запуска передачи
        // В реальной реализации здесь будет логика передачи данных
        // Для симулятора просто устанавливаем флаг завершения
        dma->isr |= (1U << channel); // Устанавливаем флаг завершения канала
    }
}

uint8_t dma_is_complete(DMA_State *dma, int channel) {
    if (channel >= 0 && channel < 7) {
        // Проверка флага завершения передачи
        return (dma->isr & (1U << channel)) ? 1 : 0;
    }
    return 0;
}