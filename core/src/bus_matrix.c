/**
 * bus_matrix.c - Bus Matrix abstraction
 *
 * Models the AHB/APB bus interconnect with shadow registers for
 * RCC clock control and DMA controller state.
 */

#include "bus_matrix.h"
#include "rcc.h"
#include "dma.h"
#include <string.h>

void bus_matrix_init(Bus_Matrix_State *bus) {
    memset(bus, 0, sizeof(Bus_Matrix_State));
    bus->ahb_base  = AHB_BASE_ADDR;
    bus->apb1_base = APB1_BASE_ADDR;
    bus->apb2_base = APB2_BASE_ADDR;
}

uint32_t bus_read(Bus_Matrix_State *bus, uint32_t addr) {
    if (!bus_is_accessible(bus, addr)) {
        return 0xFFFFFFFFU;
    }

    /* RCC registers */
    if (addr >= RCC_BASE_ADDR && addr < RCC_BASE_ADDR + 0x30U) {
        uint32_t offset = addr - RCC_BASE_ADDR;
        switch (offset) {
            case RCC_CR_OFFSET:       return bus->rcc_cr;
            case RCC_CFGR_OFFSET:     return bus->rcc_cfgr;
            case RCC_APB2RSTR_OFFSET: return bus->rcc_apb2rstr;
            case RCC_APB1RSTR_OFFSET: return bus->rcc_apb1rstr;
            case RCC_AHBENR_OFFSET:   return bus->rcc_ahbenr;
            case RCC_APB2ENR_OFFSET:  return bus->rcc_apb2enr;
            case RCC_APB1ENR_OFFSET:  return bus->rcc_apb1enr;
            default:                  return 0;
        }
    }

    /* DMA registers */
    if (addr >= DMA1_BASE_ADDR && addr < DMA1_BASE_ADDR + 0x80U) {
        uint32_t offset = addr - DMA1_BASE_ADDR;
        if (offset < 0x10U)  return bus->dma_isr;
        if (offset < 0x20U)  return bus->dma_ifcr;
        if (offset >= DMA_CCR1_OFFSET && offset < DMA_CCR1_OFFSET + 0x20U) {
            int ch = (offset - DMA_CCR1_OFFSET) / 0x14;
            if (ch >= 0 && ch < 7) return bus->dma_channels[ch].ccr;
        }
        return 0;
    }

    return 0;
}

void bus_write(Bus_Matrix_State *bus, uint32_t addr, uint32_t value) {
    if (!bus_is_accessible(bus, addr)) {
        return;
    }

    /* RCC registers */
    if (addr >= RCC_BASE_ADDR && addr < RCC_BASE_ADDR + 0x30U) {
        uint32_t offset = addr - RCC_BASE_ADDR;
        switch (offset) {
            case RCC_CR_OFFSET:       bus->rcc_cr = value;        break;
            case RCC_CFGR_OFFSET:     bus->rcc_cfgr = value;      break;
            case RCC_APB2RSTR_OFFSET: bus->rcc_apb2rstr = value;  break;
            case RCC_APB1RSTR_OFFSET: bus->rcc_apb1rstr = value;  break;
            case RCC_AHBENR_OFFSET:   bus->rcc_ahbenr = value;    break;
            case RCC_APB2ENR_OFFSET:  bus->rcc_apb2enr = value;   break;
            case RCC_APB1ENR_OFFSET:  bus->rcc_apb1enr = value;   break;
            default:                  break;
        }
        return;
    }

    /* DMA registers */
    if (addr >= DMA1_BASE_ADDR && addr < DMA1_BASE_ADDR + 0x80U) {
        uint32_t offset = addr - DMA1_BASE_ADDR;
        if (offset < 0x10U)      { bus->dma_isr = value;  return; }
        if (offset < 0x20U)      { bus->dma_ifcr = value; return; }
        if (offset >= DMA_CCR1_OFFSET && offset < DMA_CCR1_OFFSET + 0x20U) {
            int ch = (offset - DMA_CCR1_OFFSET) / 0x14;
            if (ch >= 0 && ch < 7) { bus->dma_channels[ch].ccr = value; }
        }
    }
}

uint8_t bus_is_accessible(Bus_Matrix_State *bus, uint32_t addr) {
    (void)bus;  /* accessibility is purely address-based */

    if (addr >= FLASH_BASE_ADDR && addr < FLASH_BASE_ADDR + FLASH_SIZE) return 1;
    if (addr >= SRAM_BASE_ADDR  && addr < SRAM_BASE_ADDR  + SRAM_SIZE)  return 1;
    if (addr >= AHB_BASE_ADDR   && addr < AHB_BASE_ADDR   + AHB_SIZE)   return 1;
    if (addr >= APB1_BASE_ADDR  && addr < APB1_BASE_ADDR  + APB1_SIZE)  return 1;
    if (addr >= APB2_BASE_ADDR  && addr < APB2_BASE_ADDR  + APB2_SIZE)  return 1;

    return 0;
}
