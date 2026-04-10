#ifndef RCC_H
#define RCC_H

#include <stdint.h>

// RCC Register Addresses
#define RCC_BASE_ADDR           0x40021000U
#define RCC_CR_OFFSET           0x00
#define RCC_CFGR_OFFSET         0x04
#define RCC_CIR_OFFSET          0x08
#define RCC_APB2RSTR_OFFSET     0x0C
#define RCC_APB1RSTR_OFFSET     0x10
#define RCC_AHBENR_OFFSET       0x14
#define RCC_APB2ENR_OFFSET      0x18
#define RCC_APB1ENR_OFFSET      0x1C
#define RCC_BDCR_OFFSET         0x20
#define RCC_CSR_OFFSET          0x24

// RCC_CR Register Bits
#define RCC_CR_HSION            (1U << 0)   // Internal High Speed clock enable
#define RCC_CR_HSIRDY           (1U << 1)   // Internal High Speed clock ready flag
#define RCC_CR_HSEON            (1U << 16)  // External High Speed clock enable
#define RCC_CR_HSERDY           (1U << 17)  // External High Speed clock ready flag
#define RCC_CR_PLLON            (1U << 24)  // PLL enable
#define RCC_CR_PLLRDY           (1U << 25)  // PLL clock ready flag

// RCC_CFGR Register Bits
#define RCC_CFGR_SW             (1U << 0)   // System clock switch
#define RCC_CFGR_SWS            (1U << 2)   // System clock switch status
#define RCC_CFGR_HPRE           (1U << 4)   // AHB prescaler
#define RCC_CFGR_PPRE1          (1U << 8)   // APB1 prescaler
#define RCC_CFGR_PPRE2          (1U << 11)  // APB2 prescaler
#define RCC_CFGR_ADCPRE         (1U << 14)  // ADC prescaler
#define RCC_CFGR_PLLSRC         (1U << 16)  // PLL entry clock source
#define RCC_CFGR_PLLXTPRE       (1U << 17)  // HSE divider for PLL entry
#define RCC_CFGR_PLLMUL         (1U << 18)  // PLL multiplication factor

// RCC_APB2ENR Register Bits (peripheral clocks)
#define RCC_APB2ENR_AFIOEN      (1U << 0)   // Alternate Function I/O clock enable
#define RCC_APB2ENR_IOPAEN      (1U << 2)   // I/O port A clock enable
#define RCC_APB2ENR_IOPBEN      (1U << 3)   // I/O port B clock enable
#define RCC_APB2ENR_IOPCEN      (1U << 4)   // I/O port C clock enable
#define RCC_APB2ENR_IOPDEN      (1U << 5)   // I/O port D clock enable
#define RCC_APB2ENR_IOPEEN      (1U << 6)   // I/O port E clock enable
#define RCC_APB2ENR_IOPFEN      (1U << 7)   // I/O port F clock enable
#define RCC_APB2ENR_IOPGEN      (1U << 8)   // I/O port G clock enable
#define RCC_APB2ENR_ADC1EN      (1U << 9)   // ADC1 clock enable
#define RCC_APB2ENR_ADC2EN      (1U << 10)  // ADC2 clock enable
#define RCC_APB2ENR_TIM1EN      (1U << 11)  // TIM1 clock enable
#define RCC_APB2ENR_SPI1EN      (1U << 12)  // SPI1 clock enable
#define RCC_APB2ENR_USART1EN    (1U << 14)  // USART1 clock enable

// RCC_APB1ENR Register Bits (peripheral clocks)
#define RCC_APB1ENR_TIM2EN      (1U << 0)   // TIM2 clock enable
#define RCC_APB1ENR_TIM3EN      (1U << 1)   // TIM3 clock enable
#define RCC_APB1ENR_TIM4EN      (1U << 2)   // TIM4 clock enable
#define RCC_APB1ENR_TIM5EN      (1U << 3)   // TIM5 clock enable
#define RCC_APB1ENR_TIM6EN      (1U << 4)   // TIM6 clock enable
#define RCC_APB1ENR_TIM7EN      (1U << 5)   // TIM7 clock enable
#define RCC_APB1ENR_WWDGEN      (1U << 11)  // Window Watchdog clock enable
#define RCC_APB1ENR_SPI2EN      (1U << 14)  // SPI2 clock enable
#define RCC_APB1ENR_USART2EN    (1U << 17)  // USART2 clock enable
#define RCC_APB1ENR_USART3EN    (1U << 18)  // USART3 clock enable
#define RCC_APB1ENR_USART4EN    (1U << 19)  // USART4 clock enable
#define RCC_APB1ENR_USART5EN    (1U << 20)  // USART5 clock enable
#define RCC_APB1ENR_I2C1EN      (1U << 21)  // I2C1 clock enable
#define RCC_APB1ENR_I2C2EN      (1U << 22)  // I2C2 clock enable
#define RCC_APB1ENR_CANEN       (1U << 25)  // CAN clock enable
#define RCC_APB1ENR_BKPEN       (1U << 27)  // Backup interface clock enable
#define RCC_APB1ENR_PWREN       (1U << 28)  // Power interface clock enable
#define RCC_APB1ENR_DACEN       (1U << 29)  // DAC interface clock enable

// RCC State Structure
typedef struct {
    uint32_t cr;            // Clock control register
    uint32_t cfgr;          // Clock configuration register
    uint32_t cir;           // Clock interrupt register
    uint32_t apb2rstr;      // APB2 reset register
    uint32_t apb1rstr;      // APB1 reset register
    uint32_t ahbenr;        // AHB enable register
    uint32_t apb2enr;       // APB2 enable register
    uint32_t apb1enr;       // APB1 enable register
    uint32_t bdcr;          // Backup domain control register
    uint32_t csr;           // Control/status register
} RCC_State;

// Initialize RCC state
void rcc_init(RCC_State *rcc);

// Reset RCC state
void rcc_reset(RCC_State *rcc);

// Read RCC register
uint32_t rcc_read_register(RCC_State *rcc, uint32_t addr);

// Write RCC register
void rcc_write_register(RCC_State *rcc, uint32_t addr, uint32_t value);

// Enable peripheral clock
void rcc_enable_peripheral(RCC_State *rcc, uint32_t peripheral);

// Disable peripheral clock
void rcc_disable_peripheral(RCC_State *rcc, uint32_t peripheral);

// Check if peripheral is enabled
uint8_t rcc_is_peripheral_enabled(RCC_State *rcc, uint32_t peripheral);

#endif // RCC_H