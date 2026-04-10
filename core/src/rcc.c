/**
 * rcc.c - Reset and Clock Control peripheral
 *
 * Models the RCC block that manages clock tree and peripheral enable signals.
 */

#include "rcc.h"
#include <stdio.h>
#include <string.h>

void rcc_init(RCC_State *rcc) {
    memset(rcc, 0, sizeof(RCC_State));
    /* HSI oscillator ready after reset */
    rcc->cr = RCC_CR_HSIRDY;
}

void rcc_reset(RCC_State *rcc) {
    memset(rcc, 0, sizeof(RCC_State));
    rcc->cr = RCC_CR_HSIRDY;
}

uint32_t rcc_read_register(RCC_State *rcc, uint32_t addr) {
    uint32_t offset = addr - RCC_BASE_ADDR;

    switch (offset) {
        case RCC_CR_OFFSET:        return rcc->cr;
        case RCC_CFGR_OFFSET:      return rcc->cfgr;
        case RCC_CIR_OFFSET:       return rcc->cir;
        case RCC_APB2RSTR_OFFSET:  return rcc->apb2rstr;
        case RCC_APB1RSTR_OFFSET:  return rcc->apb1rstr;
        case RCC_AHBENR_OFFSET:    return rcc->ahbenr;
        case RCC_APB2ENR_OFFSET:   return rcc->apb2enr;
        case RCC_APB1ENR_OFFSET:   return rcc->apb1enr;
        case RCC_BDCR_OFFSET:      return rcc->bdcr;
        case RCC_CSR_OFFSET:       return rcc->csr;
        default:                   return 0;
    }
}

void rcc_write_register(RCC_State *rcc, uint32_t addr, uint32_t value) {
    uint32_t offset = addr - RCC_BASE_ADDR;

    switch (offset) {
        case RCC_CR_OFFSET:        rcc->cr = value;        break;
        case RCC_CFGR_OFFSET:      rcc->cfgr = value;      break;
        case RCC_CIR_OFFSET:       rcc->cir = value;       break;
        case RCC_APB2RSTR_OFFSET:  rcc->apb2rstr = value;  break;
        case RCC_APB1RSTR_OFFSET:  rcc->apb1rstr = value;  break;
        case RCC_AHBENR_OFFSET:    rcc->ahbenr = value;    break;
        case RCC_APB2ENR_OFFSET:   rcc->apb2enr = value;   break;
        case RCC_APB1ENR_OFFSET:   rcc->apb1enr = value;   break;
        case RCC_BDCR_OFFSET:      rcc->bdcr = value;      break;
        case RCC_CSR_OFFSET:       rcc->csr = value;       break;
        default:                   break;
    }
}

void rcc_enable_peripheral(RCC_State *rcc, uint32_t peripheral) {
    if (peripheral & 0xFFFF0000U) {
        rcc->apb2enr |= peripheral;
    } else {
        rcc->apb1enr |= peripheral;
    }
}

void rcc_disable_peripheral(RCC_State *rcc, uint32_t peripheral) {
    if (peripheral & 0xFFFF0000U) {
        rcc->apb2enr &= ~peripheral;
    } else {
        rcc->apb1enr &= ~peripheral;
    }
}

uint8_t rcc_is_peripheral_enabled(RCC_State *rcc, uint32_t peripheral) {
    if (peripheral & 0xFFFF0000U) {
        return (rcc->apb2enr & peripheral) ? 1 : 0;
    } else {
        return (rcc->apb1enr & peripheral) ? 1 : 0;
    }
}
