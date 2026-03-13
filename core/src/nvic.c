#include "nvic.h"
#include <stdio.h>
#include <string.h>

void nvic_init(NVIC_State *nvic) {
    // Initialize all NVIC registers to zero
    memset(nvic, 0, sizeof(NVIC_State));
}

void nvic_reset(NVIC_State *nvic) {
    // Reset all NVIC registers
    memset(nvic, 0, sizeof(NVIC_State));
}

void nvic_enable_interrupt(NVIC_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        uint32_t reg_index = irqn / 32;
        uint32_t bit_index = irqn % 32;
        nvic->iser[reg_index] |= (1U << bit_index);
    }
}

void nvic_disable_interrupt(NVIC_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        uint32_t reg_index = irqn / 32;
        uint32_t bit_index = irqn % 32;
        nvic->icer[reg_index] |= (1U << bit_index);
    }
}

void nvic_set_pending(NVIC_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        uint32_t reg_index = irqn / 32;
        uint32_t bit_index = irqn % 32;
        nvic->ispr[reg_index] |= (1U << bit_index);
    }
}

void nvic_clear_pending(NVIC_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        uint32_t reg_index = irqn / 32;
        uint32_t bit_index = irqn % 32;
        nvic->icpr[reg_index] |= (1U << bit_index);
    }
}

uint8_t nvic_is_enabled(NVIC_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        uint32_t reg_index = irqn / 32;
        uint32_t bit_index = irqn % 32;
        return (nvic->iser[reg_index] & (1U << bit_index)) ? 1 : 0;
    }
    return 0;
}

uint8_t nvic_is_pending(NVIC_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        uint32_t reg_index = irqn / 32;
        uint32_t bit_index = irqn % 32;
        return (nvic->ispr[reg_index] & (1U << bit_index)) ? 1 : 0;
    }
    return 0;
}

void nvic_set_priority(NVIC_State *nvic, uint8_t irqn, uint8_t priority) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        nvic->ipr[irqn] = priority;
    }
}

uint8_t nvic_get_priority(NVIC_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        return nvic->ipr[irqn];
    }
    return 0;
}

uint8_t nvic_is_active(NVIC_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_NUM_INTERRUPTS) {
        uint32_t reg_index = irqn / 32;
        uint32_t bit_index = irqn % 32;
        return (nvic->iabr[reg_index] & (1U << bit_index)) ? 1 : 0;
    }
    return 0;
}