#include "usart.h"
#include <stdio.h>
#include <string.h>

void usart_init(USART_State *usart, uint32_t base_addr) {
    // Инициализация всех регистров нулями
    memset(usart, 0, sizeof(USART_State));
    
    // Установка базового адреса
    usart->base_address = base_addr;
    
    // Инициализация буферов
    usart->tx_head = 0;
    usart->tx_tail = 0;
    usart->rx_head = 0;
    usart->rx_tail = 0;
    usart->tx_count = 0;
    usart->rx_count = 0;
    
    // Инициализация регистров
    usart->regs.sr = USART_SR_TXE | USART_SR_TC;  // TXE и TC установлены по умолчанию
    usart->regs.dr = 0;
    usart->regs.brr = 0;
    usart->regs.cr1 = 0;
    usart->regs.cr2 = 0;
    usart->regs.cr3 = 0;
    usart->regs.gtpr = 0;
}

void usart_reset(USART_State *usart) {
    // Сброс всех регистров
    memset(&usart->regs, 0, sizeof(USART_Registers));
    
    // Сброс буферов
    usart->tx_head = 0;
    usart->tx_tail = 0;
    usart->rx_head = 0;
    usart->rx_tail = 0;
    usart->tx_count = 0;
    usart->rx_count = 0;
}

uint32_t usart_read_register(USART_State *usart, uint32_t addr) {
    // Вычисление смещения регистра
    uint32_t offset = addr - usart->base_address;
    
    switch (offset) {
        case USART_SR_OFFSET:
            return usart->regs.sr;
        case USART_DR_OFFSET:
            return usart->regs.dr;
        case USART_BRR_OFFSET:
            return usart->regs.brr;
        case USART_CR1_OFFSET:
            return usart->regs.cr1;
        case USART_CR2_OFFSET:
            return usart->regs.cr2;
        case USART_CR3_OFFSET:
            return usart->regs.cr3;
        case USART_GTPR_OFFSET:
            return usart->regs.gtpr;
        default:
            //printf("[USART] Reading from unknown register 0x%08X\n", addr);
            return 0;
    }
}

void usart_write_register(USART_State *usart, uint32_t addr, uint32_t value) {
    // Вычисление смещения регистра
    uint32_t offset = addr - usart->base_address;
    
    switch (offset) {
        case USART_SR_OFFSET:
            // SR регистр только для чтения (кроме некоторых флагов)
            //printf("[USART] Attempted to write to read-only register SR\n");
            break;
        case USART_DR_OFFSET:
            usart->regs.dr = value;
            // Выводим переданный байт в stdout для оркестратора
            printf("[UART] TX: 0x%02X\n", value & 0xFF);
            // При записи в DR, устанавливаем флаг TXE
            usart->regs.sr |= USART_SR_TXE;
            // Сбрасываем флаг TC
            usart->regs.sr &= ~USART_SR_TC;
            break;
        case USART_BRR_OFFSET:
            usart->regs.brr = value;
            break;
        case USART_CR1_OFFSET:
            usart->regs.cr1 = value;
            break;
        case USART_CR2_OFFSET:
            usart->regs.cr2 = value;
            break;
        case USART_CR3_OFFSET:
            usart->regs.cr3 = value;
            break;
        case USART_GTPR_OFFSET:
            usart->regs.gtpr = value;
            break;
        default:
            //printf("[USART] Writing to unknown register 0x%08X\n", addr);
            break;
    }
}

void usart_transmit(USART_State *usart, uint8_t data) {
    // Проверка возможности передачи
    if (usart->tx_count < 256) {
        // Добавляем данные в буфер передачи
        usart->tx_buffer[usart->tx_head] = data;
        usart->tx_head = (usart->tx_head + 1) % 256;
        usart->tx_count++;
        
        // Устанавливаем флаг TXE (Transmit data register empty)
        usart->regs.sr |= USART_SR_TXE;
        
        // Сбрасываем флаг TC (Transmission complete)
        usart->regs.sr &= ~USART_SR_TC;
    }
}

uint8_t usart_receive(USART_State *usart) {
    uint8_t data = 0;
    
    // Проверка наличия данных для приема
    if (usart->rx_count > 0) {
        data = usart->rx_buffer[usart->rx_tail];
        usart->rx_tail = (usart->rx_tail + 1) % 256;
        usart->rx_count--;
        
        // Если буфер пуст, сбрасываем флаг RXNE
        if (usart->rx_count == 0) {
            usart->regs.sr &= ~USART_SR_RXNE;
        }
    }
    
    return data;
}

uint8_t usart_has_data(USART_State *usart) {
    return (usart->rx_count > 0) ? 1 : 0;
}

uint8_t usart_can_transmit(USART_State *usart) {
    return (usart->tx_count < 256) ? 1 : 0;
}