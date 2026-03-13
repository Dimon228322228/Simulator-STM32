#include "rcc.h"
#include <stdio.h>
#include <string.h>

void rcc_init(RCC_State *rcc) {
    // Инициализация всех регистров нулями
    memset(rcc, 0, sizeof(RCC_State));
    
    // Установка значений по умолчанию
    rcc->cr = RCC_CR_HSIRDY;  // Внутренний高速 тактовый генератор включен и готов
    rcc->cfgr = 0;            // По умолчанию все нули
    rcc->cir = 0;             // Нет прерываний
    rcc->apb2rstr = 0;        // Нет сброса
    rcc->apb1rstr = 0;        // Нет сброса
    rcc->ahbenr = 0;          // Нет активных модулей AHB
    rcc->apb2enr = 0;         // Нет активных модулей APB2
    rcc->apb1enr = 0;         // Нет активных модулей APB1
    rcc->bdcr = 0;            // Нет резервного питания
    rcc->csr = 0;             // Нет статуса
}

void rcc_reset(RCC_State *rcc) {
    // Сброс всех регистров
    memset(rcc, 0, sizeof(RCC_State));
    
    // Установка значений по умолчанию
    rcc->cr = RCC_CR_HSIRDY;  // Внутренний高速 тактовый генератор включен и готов
}

uint32_t rcc_read_register(RCC_State *rcc, uint32_t addr) {
    // Вычисление смещения регистра
    uint32_t offset = addr - RCC_BASE_ADDR;
    
    switch (offset) {
        case RCC_CR_OFFSET:
            return rcc->cr;
        case RCC_CFGR_OFFSET:
            return rcc->cfgr;
        case RCC_CIR_OFFSET:
            return rcc->cir;
        case RCC_APB2RSTR_OFFSET:
            return rcc->apb2rstr;
        case RCC_APB1RSTR_OFFSET:
            return rcc->apb1rstr;
        case RCC_AHBENR_OFFSET:
            return rcc->ahbenr;
        case RCC_APB2ENR_OFFSET:
            return rcc->apb2enr;
        case RCC_APB1ENR_OFFSET:
            return rcc->apb1enr;
        case RCC_BDCR_OFFSET:
            return rcc->bdcr;
        case RCC_CSR_OFFSET:
            return rcc->csr;
        default:
            //printf("[RCC] Reading from unknown register 0x%08X\n", addr);
            return 0;
    }
}

void rcc_write_register(RCC_State *rcc, uint32_t addr, uint32_t value) {
    // Вычисление смещения регистра
    uint32_t offset = addr - RCC_BASE_ADDR;
    
    switch (offset) {
        case RCC_CR_OFFSET:
            rcc->cr = value;
            break;
        case RCC_CFGR_OFFSET:
            rcc->cfgr = value;
            break;
        case RCC_CIR_OFFSET:
            rcc->cir = value;
            break;
        case RCC_APB2RSTR_OFFSET:
            rcc->apb2rstr = value;
            break;
        case RCC_APB1RSTR_OFFSET:
            rcc->apb1rstr = value;
            break;
        case RCC_AHBENR_OFFSET:
            rcc->ahbenr = value;
            break;
        case RCC_APB2ENR_OFFSET:
            rcc->apb2enr = value;
            break;
        case RCC_APB1ENR_OFFSET:
            rcc->apb1enr = value;
            break;
        case RCC_BDCR_OFFSET:
            rcc->bdcr = value;
            break;
        case RCC_CSR_OFFSET:
            rcc->csr = value;
            break;
        default:
            //printf("[RCC] Writing to unknown register 0x%08X\n", addr);
            break;
    }
}

void rcc_enable_peripheral(RCC_State *rcc, uint32_t peripheral) {
    // Включение тактирования периферии
    if (peripheral & 0xFFFF0000) {
        // APB2 периферия
        rcc->apb2enr |= peripheral;
    } else {
        // APB1 периферия
        rcc->apb1enr |= peripheral;
    }
}

void rcc_disable_peripheral(RCC_State *rcc, uint32_t peripheral) {
    // Отключение тактирования периферии
    if (peripheral & 0xFFFF0000) {
        // APB2 периферия
        rcc->apb2enr &= ~peripheral;
    } else {
        // APB1 периферия
        rcc->apb1enr &= ~peripheral;
    }
}

uint8_t rcc_is_peripheral_enabled(RCC_State *rcc, uint32_t peripheral) {
    // Проверка, включена ли периферия
    if (peripheral & 0xFFFF0000) {
        // APB2 периферия
        return (rcc->apb2enr & peripheral) ? 1 : 0;
    } else {
        // APB1 периферия
        return (rcc->apb1enr & peripheral) ? 1 : 0;
    }
}