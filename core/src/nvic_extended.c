#include "nvic_extended.h"
#include <stdio.h>
#include <string.h>

void nvic_extended_init(NVIC_Extended_State *nvic) {
    // Инициализация всех векторов нулями
    memset(nvic, 0, sizeof(NVIC_Extended_State));
    
    // Инициализация векторов прерываний
    for (int i = 0; i < NVIC_IRQ_NUMBER; i++) {
        nvic->vectors[i].vector_address = 0;
        nvic->vectors[i].priority = 0;
        nvic->vectors[i].enabled = 0;
        nvic->vectors[i].pending = 0;
        nvic->vectors[i].active = 0;
    }
    
    // Установка базового приоритета
    nvic->base_priority = 0;
    nvic->active_priority = 0;
    nvic->interrupt_active = 0;
    nvic->interrupt_pending = 0;
    nvic->interrupt_enabled = 0;
}

void nvic_enable_irq(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        nvic->vectors[irqn].enabled = 1;
        nvic->interrupt_enabled |= (1U << irqn);
    }
}

void nvic_disable_irq(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        nvic->vectors[irqn].enabled = 0;
        nvic->interrupt_enabled &= ~(1U << irqn);
    }
}

void nvic_set_priority(NVIC_Extended_State *nvic, uint8_t irqn, uint8_t priority) {
    if (irqn < NVIC_IRQ_NUMBER) {
        nvic->vectors[irqn].priority = priority;
    }
}

uint8_t nvic_get_priority(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        return nvic->vectors[irqn].priority;
    }
    return 0;
}

void nvic_set_pending(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        nvic->vectors[irqn].pending = 1;
        nvic->interrupt_pending |= (1U << irqn);
    }
}

void nvic_clear_pending(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        nvic->vectors[irqn].pending = 0;
        nvic->interrupt_pending &= ~(1U << irqn);
    }
}

uint8_t nvic_is_active(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        return nvic->vectors[irqn].active;
    }
    return 0;
}

uint8_t nvic_is_enabled(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        return nvic->vectors[irqn].enabled;
    }
    return 0;
}

uint8_t nvic_is_pending(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        return nvic->vectors[irqn].pending;
    }
    return 0;
}

void nvic_handle_interrupt(NVIC_Extended_State *nvic, uint8_t irqn) {
    if (irqn < NVIC_IRQ_NUMBER) {
        // Устанавливаем прерывание как активное
        nvic->vectors[irqn].active = 1;
        nvic->vectors[irqn].pending = 0;
        nvic->interrupt_active |= (1U << irqn);
        nvic->interrupt_pending &= ~(1U << irqn);
        
        // Обновляем приоритеты
        if (nvic->vectors[irqn].priority > nvic->active_priority) {
            nvic->active_priority = nvic->vectors[irqn].priority;
        }
    }
}

uint8_t nvic_has_active_interrupt(NVIC_Extended_State *nvic) {
    return (nvic->interrupt_active != 0) ? 1 : 0;
}

uint8_t nvic_get_active_interrupt(NVIC_Extended_State *nvic) {
    // Возвращает номер первого активного прерывания
    for (int i = 0; i < NVIC_IRQ_NUMBER; i++) {
        if (nvic->vectors[i].active) {
            return i;
        }
    }
    return 0;
}