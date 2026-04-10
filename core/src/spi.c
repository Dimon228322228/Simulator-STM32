#include "spi.h"
#include <stdio.h>
#include <string.h>

void spi_init(SPI_State *spi, uint32_t base_addr) {
    // Инициализация всех регистров нулями
    memset(spi, 0, sizeof(SPI_State));
    
    // Установка базового адреса
    spi->base_address = base_addr;
    
    // Инициализация буферов
    spi->tx_head = 0;
    spi->tx_tail = 0;
    spi->rx_head = 0;
    spi->rx_tail = 0;
    spi->tx_count = 0;
    spi->rx_count = 0;
    
    // Инициализация регистров
    spi->regs.cr1 = 0;
    spi->regs.cr2 = 0;
    spi->regs.sr = SPI_SR_RXNE | SPI_SR_TXE;  // По умолчанию буферы готовы
    spi->regs.dr = 0;
    spi->regs.crcpr = 0;
    spi->regs.rxcrcr = 0;
    spi->regs.txcrcr = 0;
    spi->regs.i2scfgr = 0;
    spi->regs.i2spr = 0;
    
    // По умолчанию в режиме мастера
    spi->master_mode = 1;
    spi->enabled = 0;
}

void spi_reset(SPI_State *spi) {
    // Сброс всех регистров
    memset(&spi->regs, 0, sizeof(SPI_Registers));
    
    // Сброс буферов
    spi->tx_head = 0;
    spi->tx_tail = 0;
    spi->rx_head = 0;
    spi->rx_tail = 0;
    spi->tx_count = 0;
    spi->rx_count = 0;
    
    // Сброс состояния
    spi->master_mode = 1;
    spi->enabled = 0;
    
    // Установка флагов по умолчанию
    spi->regs.sr = SPI_SR_RXNE | SPI_SR_TXE;
}

uint32_t spi_read_register(SPI_State *spi, uint32_t addr) {
    // Вычисление смещения регистра
    uint32_t offset = addr - spi->base_address;
    
    switch (offset) {
        case SPI_CR1_OFFSET:
            return spi->regs.cr1;
        case SPI_CR2_OFFSET:
            return spi->regs.cr2;
        case SPI_SR_OFFSET:
            return spi->regs.sr;
        case SPI_DR_OFFSET:
            return spi->regs.dr;
        case SPI_CRCPR_OFFSET:
            return spi->regs.crcpr;
        case SPI_RXCRCR_OFFSET:
            return spi->regs.rxcrcr;
        case SPI_TXCRCR_OFFSET:
            return spi->regs.txcrcr;
        case SPI_I2SCFGR_OFFSET:
            return spi->regs.i2scfgr;
        case SPI_I2SPR_OFFSET:
            return spi->regs.i2spr;
        default:
            //printf("[SPI] Reading from unknown register 0x%08X\n", addr);
            return 0;
    }
}

void spi_write_register(SPI_State *spi, uint32_t addr, uint32_t value) {
    // Вычисление смещения регистра
    uint32_t offset = addr - spi->base_address;
    
    switch (offset) {
        case SPI_CR1_OFFSET:
            spi->regs.cr1 = value;
            break;
        case SPI_CR2_OFFSET:
            spi->regs.cr2 = value;
            break;
        case SPI_SR_OFFSET:
            // SR регистр только для чтения (кроме некоторых флагов)
            //printf("[SPI] Attempted to write to read-only register SR\n");
            break;
        case SPI_DR_OFFSET:
            spi->regs.dr = value;
            // При записи в DR, устанавливаем флаг TXE
            spi->regs.sr |= SPI_SR_TXE;
            // Сбрасываем флаг RXNE если буфер пуст
            if (spi->rx_count == 0) {
                spi->regs.sr &= ~SPI_SR_RXNE;
            }
            break;
        case SPI_CRCPR_OFFSET:
            spi->regs.crcpr = value;
            break;
        case SPI_RXCRCR_OFFSET:
            spi->regs.rxcrcr = value;
            break;
        case SPI_TXCRCR_OFFSET:
            spi->regs.txcrcr = value;
            break;
        case SPI_I2SCFGR_OFFSET:
            spi->regs.i2scfgr = value;
            break;
        case SPI_I2SPR_OFFSET:
            spi->regs.i2spr = value;
            break;
        default:
            //printf("[SPI] Writing to unknown register 0x%08X\n", addr);
            break;
    }
}

void spi_transmit(SPI_State *spi, uint8_t data) {
    // Проверка возможности передачи
    if (spi->tx_count < 256) {
        // Добавляем данные в буфер передачи
        spi->tx_buffer[spi->tx_head] = data;
        spi->tx_head = (spi->tx_head + 1) % 256;
        spi->tx_count++;
        
        // Устанавливаем флаг TXE (Transmit buffer empty)
        spi->regs.sr |= SPI_SR_TXE;
        
        // Сбрасываем флаг RXNE если буфер пуст
        if (spi->rx_count == 0) {
            spi->regs.sr &= ~SPI_SR_RXNE;
        }
    }
}

uint8_t spi_receive(SPI_State *spi) {
    uint8_t data = 0;
    
    // Проверка наличия данных для приема
    if (spi->rx_count > 0) {
        data = spi->rx_buffer[spi->rx_tail];
        spi->rx_tail = (spi->rx_tail + 1) % 256;
        spi->rx_count--;
        
        // Если буфер пуст, сбрасываем флаг RXNE
        if (spi->rx_count == 0) {
            spi->regs.sr &= ~SPI_SR_RXNE;
        }
    }
    
    return data;
}

uint8_t spi_has_data(SPI_State *spi) {
    return (spi->rx_count > 0) ? 1 : 0;
}

uint8_t spi_can_transmit(SPI_State *spi) {
    return (spi->tx_count < 256) ? 1 : 0;
}

void spi_set_master_mode(SPI_State *spi, uint8_t master) {
    spi->master_mode = master;
    if (master) {
        spi->regs.cr1 |= SPI_CR1_MSTR;
    } else {
        spi->regs.cr1 &= ~SPI_CR1_MSTR;
    }
}

void spi_enable(SPI_State *spi) {
    spi->enabled = 1;
    spi->regs.cr1 |= SPI_CR1_SPE;
}

void spi_disable(SPI_State *spi) {
    spi->enabled = 0;
    spi->regs.cr1 &= ~SPI_CR1_SPE;
}