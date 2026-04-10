#include "i2c.h"
#include <stdio.h>
#include <string.h>

void i2c_init(I2C_State *i2c, uint32_t base_addr) {
    // Инициализация всех регистров нулями
    memset(i2c, 0, sizeof(I2C_State));
    
    // Установка базового адреса
    i2c->base_address = base_addr;
    
    // Инициализация буферов
    i2c->tx_head = 0;
    i2c->tx_tail = 0;
    i2c->rx_head = 0;
    i2c->rx_tail = 0;
    i2c->tx_count = 0;
    i2c->rx_count = 0;
    
    // Инициализация регистров
    i2c->regs.cr1 = 0;
    i2c->regs.cr2 = 0;
    i2c->regs.oar1 = 0;
    i2c->regs.oar2 = 0;
    i2c->regs.dr = 0;
    i2c->regs.sr1 = I2C_SR1_RXNE | I2C_SR1_TXE;  // По умолчанию буферы готовы
    i2c->regs.sr2 = 0;
    i2c->regs.ccr = 0;
    i2c->regs.trise = 0;
    
    // По умолчанию в режиме мастера
    i2c->master_mode = 1;
    i2c->enabled = 0;
    i2c->address = 0;
    i2c->direction = 0;  // 0 - передача, 1 - прием
}

void i2c_reset(I2C_State *i2c) {
    // Сброс всех регистров
    memset(&i2c->regs, 0, sizeof(I2C_Registers));
    
    // Сброс буферов
    i2c->tx_head = 0;
    i2c->tx_tail = 0;
    i2c->rx_head = 0;
    i2c->rx_tail = 0;
    i2c->tx_count = 0;
    i2c->rx_count = 0;
    
    // Сброс состояния
    i2c->master_mode = 1;
    i2c->enabled = 0;
    i2c->address = 0;
    i2c->direction = 0;
    
    // Установка флагов по умолчанию
    i2c->regs.sr1 = I2C_SR1_RXNE | I2C_SR1_TXE;
    i2c->regs.sr2 = 0;
}

uint32_t i2c_read_register(I2C_State *i2c, uint32_t addr) {
    // Вычисление смещения регистра
    uint32_t offset = addr - i2c->base_address;
    
    switch (offset) {
        case I2C_CR1_OFFSET:
            return i2c->regs.cr1;
        case I2C_CR2_OFFSET:
            return i2c->regs.cr2;
        case I2C_OAR1_OFFSET:
            return i2c->regs.oar1;
        case I2C_OAR2_OFFSET:
            return i2c->regs.oar2;
        case I2C_DR_OFFSET:
            return i2c->regs.dr;
        case I2C_SR1_OFFSET:
            return i2c->regs.sr1;
        case I2C_SR2_OFFSET:
            return i2c->regs.sr2;
        case I2C_CCR_OFFSET:
            return i2c->regs.ccr;
        case I2C_TRISE_OFFSET:
            return i2c->regs.trise;
        default:
            //printf("[I2C] Reading from unknown register 0x%08X\n", addr);
            return 0;
    }
}

void i2c_write_register(I2C_State *i2c, uint32_t addr, uint32_t value) {
    // Вычисление смещения регистра
    uint32_t offset = addr - i2c->base_address;
    
    switch (offset) {
        case I2C_CR1_OFFSET:
            i2c->regs.cr1 = value;
            break;
        case I2C_CR2_OFFSET:
            i2c->regs.cr2 = value;
            break;
        case I2C_OAR1_OFFSET:
            i2c->regs.oar1 = value;
            break;
        case I2C_OAR2_OFFSET:
            i2c->regs.oar2 = value;
            break;
        case I2C_DR_OFFSET:
            i2c->regs.dr = value;
            // При записи в DR, устанавливаем флаг TXE
            i2c->regs.sr1 |= I2C_SR1_TXE;
            // Сбрасываем флаг RXNE если буфер пуст
            if (i2c->rx_count == 0) {
                i2c->regs.sr1 &= ~I2C_SR1_RXNE;
            }
            break;
        case I2C_SR1_OFFSET:
            // SR1 регистр только для чтения (кроме некоторых флагов)
            //printf("[I2C] Attempted to write to read-only register SR1\n");
            break;
        case I2C_SR2_OFFSET:
            // SR2 регистр только для чтения
            //printf("[I2C] Attempted to write to read-only register SR2\n");
            break;
        case I2C_CCR_OFFSET:
            i2c->regs.ccr = value;
            break;
        case I2C_TRISE_OFFSET:
            i2c->regs.trise = value;
            break;
        default:
            //printf("[I2C] Writing to unknown register 0x%08X\n", addr);
            break;
    }
}

void i2c_transmit(I2C_State *i2c, uint8_t data) {
    // Проверка возможности передачи
    if (i2c->tx_count < 256) {
        // Добавляем данные в буфер передачи
        i2c->tx_buffer[i2c->tx_head] = data;
        i2c->tx_head = (i2c->tx_head + 1) % 256;
        i2c->tx_count++;
        
        // Устанавливаем флаг TXE (Data Register Empty)
        i2c->regs.sr1 |= I2C_SR1_TXE;
        
        // Сбрасываем флаг RXNE если буфер пуст
        if (i2c->rx_count == 0) {
            i2c->regs.sr1 &= ~I2C_SR1_RXNE;
        }
    }
}

uint8_t i2c_receive(I2C_State *i2c) {
    uint8_t data = 0;
    
    // Проверка наличия данных для приема
    if (i2c->rx_count > 0) {
        data = i2c->rx_buffer[i2c->rx_tail];
        i2c->rx_tail = (i2c->rx_tail + 1) % 256;
        i2c->rx_count--;
        
        // Если буфер пуст, сбрасываем флаг RXNE
        if (i2c->rx_count == 0) {
            i2c->regs.sr1 &= ~I2C_SR1_RXNE;
        }
    }
    
    return data;
}

uint8_t i2c_has_data(I2C_State *i2c) {
    return (i2c->rx_count > 0) ? 1 : 0;
}

uint8_t i2c_can_transmit(I2C_State *i2c) {
    return (i2c->tx_count < 256) ? 1 : 0;
}

void i2c_set_master_mode(I2C_State *i2c, uint8_t master) {
    i2c->master_mode = master;
    if (master) {
        i2c->regs.cr1 |= I2C_CR1_PE;
    } else {
        i2c->regs.cr1 &= ~I2C_CR1_PE;
    }
}

void i2c_enable(I2C_State *i2c) {
    i2c->enabled = 1;
    i2c->regs.cr1 |= I2C_CR1_PE;
}

void i2c_disable(I2C_State *i2c) {
    i2c->enabled = 0;
    i2c->regs.cr1 &= ~I2C_CR1_PE;
}

void i2c_set_address(I2C_State *i2c, uint8_t address) {
    i2c->address = address;
}