#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "memory_map.h"

// SPI Register Addresses
#define SPI1_BASE_ADDR          0x40013000U
#define SPI2_BASE_ADDR          0x40003800U
#define SPI3_BASE_ADDR          0x40003C00U

// SPI Registers
#define SPI_CR1_OFFSET            0x00
#define SPI_CR2_OFFSET            0x04
#define SPI_SR_OFFSET             0x08
#define SPI_DR_OFFSET             0x0C
#define SPI_CRCPR_OFFSET          0x10
#define SPI_RXCRCR_OFFSET         0x14
#define SPI_TXCRCR_OFFSET         0x18
#define SPI_I2SCFGR_OFFSET        0x1C
#define SPI_I2SPR_OFFSET          0x20

// SPI Control Register 1 (CR1)
#define SPI_CR1_CPHA              (1U << 0)   // Clock Phase
#define SPI_CR1_CPOL              (1U << 1)   // Clock Polarity
#define SPI_CR1_MSTR              (1U << 2)   // Master Selection
#define SPI_CR1_BR                (1U << 3)   // Baud Rate Control
#define SPI_CR1_SPE               (1U << 6)   // SPI Enable
#define SPI_CR1_LSBFIRST          (1U << 7)   // Frame Format
#define SPI_CR1_SSI               (1U << 8)   // Internal slave select
#define SPI_CR1_SSM               (1U << 9)   // Software slave management
#define SPI_CR1_RXONLY            (1U << 10)  // Receive only
#define SPI_CR1_DFF               (1U << 11)  // Data Frame Format
#define SPI_CR1_CRCNEXT           (1U << 12)  // CRC Transfer
#define SPI_CR1_CRCEN             (1U << 13)  // Hardware CRC calculation enable
#define SPI_CR1_BIDIOE            (1U << 14)  // Output enable in bidirectional mode
#define SPI_CR1_BIDIMODE          (1U << 15)  // Bidirectional data mode enable

// SPI Control Register 2 (CR2)
#define SPI_CR2_RXDMAEN           (1U << 0)   // Rx Buffer DMA Enable
#define SPI_CR2_TXDMAEN           (1U << 1)   // Tx Buffer DMA Enable
#define SPI_CR2_SSOE              (1U << 2)   // SS Output Enable
#define SPI_CR2_FRF               (1U << 4)   // Frame Format
#define SPI_CR2_ERRIE             (1U << 5)   // Error Interrupt Enable
#define SPI_CR2_RXNEIE            (1U << 6)   // RXNE Interrupt Enable
#define SPI_CR2_TXEIE             (1U << 7)   // TXE Interrupt Enable

// SPI Status Register (SR)
#define SPI_SR_RXNE               (1U << 0)   // Receive buffer not empty
#define SPI_SR_TXE                (1U << 1)   // Transmit buffer empty
#define SPI_SR_CHSIDE             (1U << 2)   // Channel side
#define SPI_SR_UDR                (1U << 3)   // Underrun flag
#define SPI_SR_CRCERR             (1U << 4)   // CRC Error flag
#define SPI_SR_MODF               (1U << 5)   // Mode fault
#define SPI_SR_OVR                (1U << 6)   // Overrun flag
#define SPI_SR_BSY                (1U << 7)   // Busy flag

// SPI Structure
typedef struct {
    uint32_t cr1;       // Control register 1
    uint32_t cr2;       // Control register 2
    uint32_t sr;        // Status register
    uint32_t dr;        // Data register
    uint32_t crcpr;     // CRC polynomial register
    uint32_t rxcrcr;    // RX CRC register
    uint32_t txcrcr;    // TX CRC register
    uint32_t i2scfgr;   // I2S configuration register
    uint32_t i2spr;     // I2S prescaler register
} SPI_Registers;

// SPI State Structure
typedef struct {
    SPI_Registers regs;
    uint32_t base_address;
    uint8_t tx_buffer[256];  // Буфер передачи
    uint8_t rx_buffer[256];  // Буфер приема
    uint32_t tx_head;        // Указатель головы буфера передачи
    uint32_t tx_tail;        // Указатель хвоста буфера передачи
    uint32_t rx_head;        // Указатель головы буфера приема
    uint32_t rx_tail;        // Указатель хвоста буфера приема
    uint32_t tx_count;       // Количество байт в буфере передачи
    uint32_t rx_count;       // Количество байт в буфере приема
    uint8_t master_mode;     // Режим мастера
    uint8_t enabled;         // Флаг включения SPI
} SPI_State;

// Инициализация SPI
void spi_init(SPI_State *spi, uint32_t base_addr);

// Сброс SPI
void spi_reset(SPI_State *spi);

// Чтение регистра SPI
uint32_t spi_read_register(SPI_State *spi, uint32_t addr);

// Запись в регистр SPI
void spi_write_register(SPI_State *spi, uint32_t addr, uint32_t value);

// Отправка данных через SPI
void spi_transmit(SPI_State *spi, uint8_t data);

// Прием данных через SPI
uint8_t spi_receive(SPI_State *spi);

// Проверка наличия данных для приема
uint8_t spi_has_data(SPI_State *spi);

// Проверка возможности отправки данных
uint8_t spi_can_transmit(SPI_State *spi);

// Установка режима мастера
void spi_set_master_mode(SPI_State *spi, uint8_t master);

// Включение SPI
void spi_enable(SPI_State *spi);

// Выключение SPI
void spi_disable(SPI_State *spi);

#endif // SPI_H