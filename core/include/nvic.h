#ifndef NVIC_H
#define NVIC_H

#include <stdint.h>

// NVIC Register Addresses (simplified for simulation)
#define NVIC_BASE_ADDR          0xE000E100U
#define NVIC_ISER_OFFSET        0x000  // Interrupt Set Enable Register
#define NVIC_ICER_OFFSET        0x080  // Interrupt Clear Enable Register
#define NVIC_ISPR_OFFSET        0x100  // Interrupt Set Pending Register
#define NVIC_ICPR_OFFSET        0x180  // Interrupt Clear Pending Register
#define NVIC_IABR_OFFSET        0x200  // Interrupt Active Bit Register
#define NVIC_IPR_OFFSET         0x300  // Interrupt Priority Register

// Number of interrupts supported (simplified)
#define NVIC_NUM_INTERRUPTS     80

// NVIC State Structure
typedef struct {
    uint32_t iser[NVIC_NUM_INTERRUPTS/32];  // Interrupt Set Enable Registers
    uint32_t icer[NVIC_NUM_INTERRUPTS/32];  // Interrupt Clear Enable Registers
    uint32_t ispr[NVIC_NUM_INTERRUPTS/32];  // Interrupt Set Pending Registers
    uint32_t icpr[NVIC_NUM_INTERRUPTS/32];  // Interrupt Clear Pending Registers
    uint32_t iabr[NVIC_NUM_INTERRUPTS/32];  // Interrupt Active Bit Registers
    uint8_t ipr[NVIC_NUM_INTERRUPTS];       // Interrupt Priority Registers
} NVIC_State;

// Initialize NVIC state
void nvic_init(NVIC_State *nvic);

// Reset NVIC state
void nvic_reset(NVIC_State *nvic);

// Enable interrupt
void nvic_enable_interrupt(NVIC_State *nvic, uint8_t irqn);

// Disable interrupt
void nvic_disable_interrupt(NVIC_State *nvic, uint8_t irqn);

// Set interrupt pending
void nvic_set_pending(NVIC_State *nvic, uint8_t irqn);

// Clear interrupt pending
void nvic_clear_pending(NVIC_State *nvic, uint8_t irqn);

// Check if interrupt is enabled
uint8_t nvic_is_enabled(NVIC_State *nvic, uint8_t irqn);

// Check if interrupt is pending
uint8_t nvic_is_pending(NVIC_State *nvic, uint8_t irqn);

// Set interrupt priority
void nvic_set_priority(NVIC_State *nvic, uint8_t irqn, uint8_t priority);

// Get interrupt priority
uint8_t nvic_get_priority(NVIC_State *nvic, uint8_t irqn);

// Check if interrupt is active
uint8_t nvic_is_active(NVIC_State *nvic, uint8_t irqn);

#endif // NVIC_H