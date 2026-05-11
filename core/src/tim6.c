#include "tim6.h"
#include <stdio.h>
#include <string.h>

void tim6_init(TIM6_State *tim6) {
    // Initialize all registers to zero
    memset(tim6, 0, sizeof(TIM6_State));
    
    // Set base address
    tim6->base_address = TIM6_BASE_ADDR;
    tim6->psc_counter = 0;
    tim6->update_event_count = 0;
}

void tim6_reset(TIM6_State *tim6) {
    // Reset all TIM6 registers
    tim6->regs.cr1 = 0;
    tim6->regs.cr2 = 0;
    tim6->regs.smcr = 0;
    tim6->regs.dier = 0;
    tim6->regs.sr = 0;
    tim6->regs.egr = 0;
    tim6->regs.cnt = 0;
    tim6->regs.psc = 0;
    tim6->regs.arr = 0;
    tim6->psc_counter = 0;
    tim6->update_event_count = 0;
}

// Helper function to get register offset from address
static uint32_t get_tim6_register_offset(uint32_t addr) {
    if (addr >= TIM6_BASE_ADDR && addr < TIM6_BASE_ADDR + 0x30) {
        return addr - TIM6_BASE_ADDR;
    }
    return 0xFFFFFFFF; // Invalid address
}

uint32_t tim6_read_register(TIM6_State *tim6, uint32_t addr) {
    uint32_t reg_offset = get_tim6_register_offset(addr);
    
    if (reg_offset == 0xFFFFFFFF) {
        //printf("[TIM6] Reading from invalid address 0x%08X\n", addr);
        return 0xFFFFFFFF; // Return error value
    }
    
    switch (reg_offset) {
        case TIM6_CR1_OFFSET:
            return tim6->regs.cr1;
        case TIM6_CR2_OFFSET:
            return tim6->regs.cr2;
        case TIM6_SMCR_OFFSET:
            return tim6->regs.smcr;
        case TIM6_DIER_OFFSET:
            return tim6->regs.dier;
        case TIM6_SR_OFFSET:
            return tim6->regs.sr;
        case TIM6_EGR_OFFSET:
            return tim6->regs.egr;
        case TIM6_CNT_OFFSET:
            return tim6->regs.cnt;
        case TIM6_PSC_OFFSET:
            return tim6->regs.psc;
        case TIM6_ARR_OFFSET:
            return tim6->regs.arr;
        default:
            //printf("[TIM6] Reading from unknown register offset 0x%02X\n", reg_offset);
            return 0xFFFFFFFF;
    }
}

void tim6_write_register(TIM6_State *tim6, uint32_t addr, uint32_t value) {
    uint32_t reg_offset = get_tim6_register_offset(addr);
    
    if (reg_offset == 0xFFFFFFFF) {
        //printf("[TIM6] Writing to invalid address 0x%08X\n", addr);
        return;
    }
    
    switch (reg_offset) {
        case TIM6_CR1_OFFSET:
            tim6->regs.cr1 = value;
            break;
        case TIM6_CR2_OFFSET:
            tim6->regs.cr2 = value;
            break;
        case TIM6_SMCR_OFFSET:
            tim6->regs.smcr = value;
            break;
        case TIM6_DIER_OFFSET:
            tim6->regs.dier = value;
            break;
        case TIM6_SR_OFFSET:
            // Clear flags by writing 1 to them
            tim6->regs.sr &= ~value;
            break;
        case TIM6_EGR_OFFSET:
            // Generate events
            if (value & TIM6_EGR_UG) {
                // Update generation always raises UIF (IRQ path decided separately)
                tim6->regs.sr |= TIM6_SR_UIF;
            }
            if (value & TIM6_EGR_TG) {
                // Trigger generation
                tim6->regs.sr |= TIM6_SR_TIF;
            }
            break;
        case TIM6_CNT_OFFSET:
            tim6->regs.cnt = value;
            break;
        case TIM6_PSC_OFFSET:
            tim6->regs.psc = value;
            break;
        case TIM6_ARR_OFFSET:
            tim6->regs.arr = value;
            break;
        default:
            //printf("[TIM6] Writing to unknown register offset 0x%02X\n", reg_offset);
            break;
    }
}

void tim6_update_counter(TIM6_State *tim6) {
    if (tim6->regs.cr1 & TIM6_CR1_CEN) {
        tim6->psc_counter++;

        if (tim6->psc_counter >= (tim6->regs.psc + 1U)) {
            tim6->psc_counter = 0;
            tim6->regs.cnt++;

            if (tim6->regs.cnt > tim6->regs.arr) {
                tim6->regs.cnt = 0;
                tim6->regs.sr |= TIM6_SR_UIF;
                tim6->update_event_count++;
            }
        }
    }
}

void tim6_update_irq_pending(TIM6_State *tim6, NVIC_State *nvic) {
    uint8_t should_pend = ((tim6->regs.sr & TIM6_SR_UIF) != 0U) &&
                          ((tim6->regs.dier & TIM6_DIER_UIE) != 0U);

    if (should_pend) {
        nvic_set_pending(nvic, NVIC_IRQ_TIM6);
    } else {
        nvic_clear_pending(nvic, NVIC_IRQ_TIM6);
    }
}
