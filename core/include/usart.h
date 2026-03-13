#ifndef USART_H
#define USART_H

#include <stdint.h>
#include "memory_map.h"

// USART Register Addresses
#define USART1_BASE_ADDR        0x40011000U
#define USART2_BASE_ADDR        0x40004400U
#define USART3_BASE_ADDR        0x40004800U

// USART Registers
#define USART_SR_OFFSET           0x00
#define USART_DR_OFFSET           0x04
#define USART_BRR_OFFSET          0x08
#define USART_CR1_OFFSET          0x0C
#define USART_CR2_OFFSET          0x10
#define USART_CR3_OFFSET          0x14
#define USART_GTPR_OFFSET         0x18

// USART Status Register (SR)
#define USART_SR_PE               (1U << 0)   // Parity Error
#define USART_SR_FE               (1U << 1)   // Framing Error
#define USART_SR_NE               (1U << 2)   // Noise Error
#define USART_SR_ORE              (1U << 3)   // Overrun Error
#define USART_SR_IDLE             (1U << 4)   // IDLE line detected
#define USART_SR_RXNE             (1U << 5)   // Read data register not empty
#define USART_SR_TC               (1U << 6)   // Transmission complete
#define USART_SR_TXE              (1U << 7)   // Transmit data register empty
#define USART_SR_LBD              (1U << 8)   // LIN break detection
#define USART_SR_CTS              (1U << 9)   // CTS flag

// USART Control Register 1 (CR1)
#define USART_CR1_SBK             (1U << 0)   // Send break
#define USART_CR1_RWU             (1U << 1)   // Receiver wakeup
#define USART_CR1_RE              (1U << 2)   // Receiver enable
#define USART_CR1_TE              (1U << 3)   // Transmitter enable
#define USART_CR1_IDLEIE          (1U << 4)   // IDLE interrupt enable
#define USART_CR1_RXNEIE          (1U << 5)   // RXNE interrupt enable
#define USART_CR1_TCIE            (1U << 6)   // Transmission complete interrupt enable
#define USART_CR1_TXEIE           (1U << 7)   // TXE interrupt enable
#define USART_CR1_PEIE            (1U << 8)   // PE interrupt enable
#define USART_CR1_PS              (1U << 9)   // Parity selection
#define USART_CR1_PCE             (1U << 10)  // Parity control enable
#define USART_CR1_WAKE            (1U << 11)  // Wakeup method
#define USART_CR1_M               (1U << 12)  // Word length
#define USART_CR1_UE              (1U << 13)  // USART enable

// USART Control Register 2 (CR2)
#define USART_CR2_ADD             (1U << 0)   // Address of the USART node
#define USART_CR2_LBDL            (1U << 5)   // LIN break detection length
#define USART_CR2_LBDIE           (1U << 6)   // LIN break detection interrupt enable
#define USART_CR2_LBCL            (1U << 8)   // Last bit clock pulse
#define USART_CR2_CPHA            (1U << 9)   // Clock phase
#define USART_CR2_CPOL            (1U << 10)  // Clock polarity
#define USART_CR2_CLKEN           (1U << 11)  // Clock enable
#define USART_CR2_STOP            (1U << 12)  // STOP bits
#define USART_CR2_LINEN           (1U << 14)  // LIN mode enable

// USART Control Register 3 (CR3)
#define USART_CR3_EIE             (1U << 0)   // Error interrupt enable
#define USART_CR3_IREN            (1U << 1)   // IrDA mode enable
#define USART_CR3_IRLP            (1U << 2)   // IrDA low-power
#define USART_CR3_HDSEL           (1U << 3)   // Half-duplex selection
#define USART_CR3_NACK            (1U << 4)   // Smartcard NACK enable
#define USART_CR3_SCEN            (1U << 5)   // Smartcard mode enable
#define USART_CR3_DMAR            (1U << 6)   // DMA enable receiver
#define USART_CR3_DMAT            (1U << 7)   // DMA enable transmitter
#define USART_CR3_RTSE            (1U << 8)   // RTS enable
#define USART_CR3_CTSE            (1U << 9)   // CTS enable
#define USART_CR3_CTSIE           (1U << 10)  // CTS interrupt enable

// USART Structure
typedef struct {
    uint32_t sr;        // Status register
    uint32_t dr;        // Data register
    uint32_t brr;       // Baud rate register
    uint32_t cr1;       // Control register 1
    uint32_t cr2;       // Control register 2
    uint32_t cr3;       // Control register 3
    uint32_t gtpr;      // Guard time and prescaler register
} USART_Registers;

// USART State Structure
typedef struct {
    USART_Registers regs;
    uint32_t base_address;
    uint8_t tx_buffer[256];  // Буфер передачи
    uint8_t rx_buffer[256];  // Буфер приема
    uint32_t tx_head;        // Указатель головы буфера передачи
    uint32_t tx_tail;        // Указатель хвоста буфера передачи
    uint32_t rx_head;        // Указатель головы буфера приема
    uint32_t rx_tail;        // Указатель хвоста буфера приема
    uint32_t tx_count;       // Количество байт в буфере передачи
    uint32_t rx_count;       // Количество байт в буфере приема
} USART_State;

// Инициализация USART
void usart_init(USART_State *usart, uint32_t base_addr);

// Сброс USART
void usart_reset(USART_State *usart);

// Чтение регистра USART
uint32_t usart_read_register(USART_State *usart, uint32_t addr);

// Запись в регистр USART
void usart_write_register(USART_State *usart, uint32_t addr, uint32_t value);

// Отправка данных через USART
void usart_transmit(USART_State *usart, uint8_t data);

// Прием данных через USART
uint8_t usart_receive(USART_State *usart);

// Проверка наличия данных для приема
uint8_t usart_has_data(USART_State *usart);

// Проверка возможности отправки данных
uint8_t usart_can_transmit(USART_State *usart);

#endif // USART_H