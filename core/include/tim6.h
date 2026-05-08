#ifndef TIM6_H
#define TIM6_H

#include <stdint.h>
#include "memory_map.h"
#include "nvic.h"
#include "nvic_extended.h"

// TIM6 Register Addresses
#define TIM6_BASE_ADDR          0x40001400U
#define TIM6_CR1_OFFSET         0x00
#define TIM6_CR2_OFFSET         0x04
#define TIM6_SMCR_OFFSET        0x08
#define TIM6_DIER_OFFSET        0x0C
#define TIM6_SR_OFFSET          0x10
#define TIM6_EGR_OFFSET         0x14
#define TIM6_CNT_OFFSET         0x24
#define TIM6_PSC_OFFSET         0x28
#define TIM6_ARR_OFFSET         0x2C

// TIM6 Control Register 1 (CR1)
#define TIM6_CR1_CEN            (1U << 0)  // Counter enable
#define TIM6_CR1_UDIS           (1U << 1)  // Update disable
#define TIM6_CR1_URS            (1U << 2)  // Update request source
#define TIM6_CR1_OPM            (1U << 3)  // One-pulse mode
#define TIM6_CR1_DIR            (1U << 4)  // Direction
#define TIM6_CR1_CMS            (1U << 5)  // Center-aligned mode selection
#define TIM6_CR1_ARPE           (1U << 7)  // Auto-reload preload enable

// TIM6 Control Register 2 (CR2)
#define TIM6_CR2_MMS            (1U << 4)  // Master mode selection
#define TIM6_CR2_TI1S           (1U << 7)  // TI1 selection

// TIM6 Slave Mode Control Register (SMCR)
#define TIM6_SMCR_TS            (1U << 0)  // Trigger selection
#define TIM6_SMCR_MSM           (1U << 3)  // Master/slave mode
#define TIM6_SMCR_ETPS          (1U << 4)  // External trigger prescaler
#define TIM6_SMCR_ETRF          (1U << 5)  // External trigger filter
#define TIM6_SMCR_ETP           (1U << 6)  // External trigger polarity
#define TIM6_SMCR_ECE           (1U << 7)  // External clock enable

// TIM6 DMA/Interrupt Enable Register (DIER)
#define TIM6_DIER_UIE           (1U << 4)  // Update interrupt enable
#define TIM6_DIER_UDE           (1U << 5)  // Update DMA enable
#define TIM6_DIER_TIE           (1U << 6)  // Trigger interrupt enable
#define TIM6_DIER_TDE           (1U << 7)  // Trigger DMA enable

// TIM6 Status Register (SR)
#define TIM6_SR_UIF             (1U << 0)  // Update interrupt flag
#define TIM6_SR_TIF             (1U << 1)  // Trigger interrupt flag

// TIM6 Event Generation Register (EGR)
#define TIM6_EGR_UG             (1U << 0)  // Update generation
#define TIM6_EGR_TG             (1U << 1)  // Trigger generation

// TIM6 Register Structure
typedef struct {
    uint32_t cr1;       // Control Register 1
    uint32_t cr2;       // Control Register 2
    uint32_t smcr;      // Slave Mode Control Register
    uint32_t dier;      // DMA/Interrupt Enable Register
    uint32_t sr;        // Status Register
    uint32_t egr;       // Event Generation Register
    uint32_t cnt;       // Counter
    uint32_t psc;       // Prescaler
    uint32_t arr;       // Auto Reload Register
} TIM6_Registers;

// TIM6 State Structure
typedef struct {
    TIM6_Registers regs;
    uint32_t base_address;
    uint32_t psc_counter;
} TIM6_State;

// Initialize TIM6 state
void tim6_init(TIM6_State *tim6);

// Reset TIM6 state
void tim6_reset(TIM6_State *tim6);

// Read TIM6 register
uint32_t tim6_read_register(TIM6_State *tim6, uint32_t addr);

// Write TIM6 register
void tim6_write_register(TIM6_State *tim6, uint32_t addr, uint32_t value);

// Update TIM6 counter (called periodically)
void tim6_update_counter(TIM6_State *tim6);

// Recalculate and propagate TIM6 IRQ pending into NVIC
void tim6_update_irq_pending(TIM6_State *tim6, NVIC_State *nvic);

#endif // TIM6_H
