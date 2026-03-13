#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "memory_map.h"

// I2C Register Addresses
#define I2C1_BASE_ADDR          0x40005400U
#define I2C2_BASE_ADDR          0x40005800U

// I2C Registers
#define I2C_CR1_OFFSET            0x00
#define I2C_CR2_OFFSET            0x04
#define I2C_OAR1_OFFSET           0x08
#define I2C_OAR2_OFFSET           0x0C
#define I2C_DR_OFFSET             0x10
#define I2C_SR1_OFFSET            0x14
#define I2C_SR2_OFFSET            0x18
#define I2C_CCR_OFFSET            0x1C
#define I2C_TRISE_OFFSET          0x20

// I2C Control Register 1 (CR1)
#define I2C_CR1_PE                (1U << 0)   // Peripheral Enable
#define I2C_CR1_SMBUS             (1U << 1)   // SMBus Mode
#define I2C_CR1_SMBTYPE           (1U << 3)   // SMBus Type
#define I2C_CR1_ENARP             (1U << 4)   // ARP Enable
#define I2C_CR1_ENPEC             (1U << 5)   // PEC Enable
#define I2C_CR1_ENGC              (1U << 6)   // General Call Enable
#define I2C_CR1_NOSTRETCH         (1U << 7)   // Clock Stretching Disable
#define I2C_CR1_START             (1U << 8)   // Start Generation
#define I2C_CR1_STOP              (1U << 9)   // Stop Generation
#define I2C_CR1_ACK               (1U << 10)  // Acknowledge Enable
#define I2C_CR1_POS               (1U << 11)  // Acknowledge/PEC Position
#define I2C_CR1_PEC               (1U << 12)  // Packet Error Checking
#define I2C_CR1_ALERT             (1U << 13)  // SMBus Alert
#define I2C_CR1_SWRST             (1U << 15)  // Software Reset

// I2C Control Register 2 (CR2)
#define I2C_CR2_FREQ              (1U << 0)   // Peripheral Clock Frequency
#define I2C_CR2_ITERREN           (1U << 8)   // Error Interrupt Enable
#define I2C_CR2_ITEVTEN           (1U << 9)   // Event Interrupt Enable
#define I2C_CR2_ITBUFEN           (1U << 10)  // Buffer Interrupt Enable
#define I2C_CR2_DMAEN             (1U << 11)  // DMA Requests Enable
#define I2C_CR2_LAST              (1U << 12)  // DMA Last Transfer

// I2C Status Register 1 (SR1)
#define I2C_SR1_SB                (1U << 0)   // Start Bit (Master mode)
#define I2C_SR1_ADDR              (1U << 1)   // Address Sent (Master mode) / Address matched (Slave mode)
#define I2C_SR1_BTF               (1U << 2)   // Byte Transfer Finished
#define I2C_SR1_ADD10             (1U << 3)   // 10-bit Header Sent (Master mode)
#define I2C_SR1_STOPF             (1U << 4)   // Stop Detection (Slave mode)
#define I2C_SR1_RXNE              (1U << 6)   // Data Register Not Empty (Receive mode)
#define I2C_SR1_TXE               (1U << 7)   // Data Register Empty (Transmit mode)
#define I2C_SR1_BERR              (1U << 8)   // Bus Error
#define I2C_SR1_ARLO              (1U << 9)   // Arbitration Lost (Master mode)
#define I2C_SR1_AF                (1U << 10)  // Acknowledge Failure
#define I2C_SR1_OVR               (1U << 11)  // Overrun/Underrun
#define I2C_SR1_PECERR            (1U << 12)  // PEC Error in reception
#define I2C_SR1_TIMEOUT           (1U << 14)  // Timeout or Tlow Error
#define I2C_SR1_SMBALERT          (1U << 15)  // SMBus Alert

// I2C Status Register 2 (SR2)
#define I2C_SR2_MSL               (1U << 0)   // Master/Slave
#define I2C_SR2_BUSY              (1U << 1)   // Bus Busy
#define I2C_SR2_TRA               (1U << 2)   // Transmitter/Receiver
#define I2C_SR2_GENCALL           (1U << 4)   // General Call Address (Slave mode)
#define I2C_SR2_SMBDEFAULT        (1U << 5)   // SMBus Device Default Address (Slave mode)
#define I2C_SR2_SMBHOST           (1U << 6)   // SMBus Host Header (Slave mode)
#define I2C_SR2_DUALF             (1U << 7)   // Dual Flag (Slave mode)
#define I2C_SR2_PEC               (1U << 8)   // Packet Error Checking Register

// I2C Structure
typedef struct {
    uint32_t cr1;       // Control register 1
    uint32_t cr2;       // Control register 2
    uint32_t oar1;      // Own address register 1
    uint32_t oar2;      // Own address register 2
    uint32_t dr;        // Data register
    uint32_t sr1;       // Status register 1
    uint32_t sr2;       // Status register 2
    uint32_t ccr;       // Clock control register
    uint32_t trise;     // TRISE register
} I2C_Registers;

// I2C State Structure
typedef struct {
    I2C_Registers regs;
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
    uint8_t enabled;         // Флаг включения I2C
    uint8_t address;         // Адрес устройства
    uint8_t direction;       // Направление (передача/прием)
} I2C_State;

// Инициализация I2C
void i2c_init(I2C_State *i2c, uint32_t base_addr);

// Сброс I2C
void i2c_reset(I2C_State *i2c);

// Чтение регистра I2C
uint32_t i2c_read_register(I2C_State *i2c, uint32_t addr);

// Запись в регистр I2C
void i2c_write_register(I2C_State *i2c, uint32_t addr, uint32_t value);

// Отправка данных через I2C
void i2c_transmit(I2C_State *i2c, uint8_t data);

// Прием данных через I2C
uint8_t i2c_receive(I2C_State *i2c);

// Проверка наличия данных для приема
uint8_t i2c_has_data(I2C_State *i2c);

// Проверка возможности отправки данных
uint8_t i2c_can_transmit(I2C_State *i2c);

// Установка режима мастера
void i2c_set_master_mode(I2C_State *i2c, uint8_t master);

// Включение I2C
void i2c_enable(I2C_State *i2c);

// Выключение I2C
void i2c_disable(I2C_State *i2c);

// Установка адреса устройства
void i2c_set_address(I2C_State *i2c, uint8_t address);

#endif // I2C_H