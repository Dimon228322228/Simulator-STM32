#ifndef BUS_MATRIX_H
#define BUS_MATRIX_H

#include <stdint.h>
#include "memory_map.h"

/*
 * Bus Matrix – models the AHB/APB bus interconnect.
 *
 * Peripheral base addresses are defined in their own headers:
 *   GPIO   – gpio.h
 *   TIM6   – tim6.h
 *   RCC    – rcc.h
 *   DMA    – dma.h
 *   USART  – usart.h
 *   SPI    – spi.h
 *   I2C    – i2c.h
 */

/* Bus address ranges */
#define AHB_BASE_ADDR   0x40000000U
#define AHB_SIZE        0x40000000U   /* 1 GB */
#define APB1_BASE_ADDR  0x40000000U
#define APB1_SIZE       0x00010000U   /* 64 KB */
#define APB2_BASE_ADDR  0x40010000U
#define APB2_SIZE       0x00010000U   /* 64 KB */

/** Bus Matrix state – tracks clock and DMA routing information. */
typedef struct {
    uint32_t ahb_base;
    uint32_t apb1_base;
    uint32_t apb2_base;

    /* RCC clock control shadow registers */
    uint32_t rcc_cr;
    uint32_t rcc_cfgr;
    uint32_t rcc_apb2rstr;
    uint32_t rcc_apb1rstr;
    uint32_t rcc_ahbenr;
    uint32_t rcc_apb2enr;
    uint32_t rcc_apb1enr;

    /* DMA controller state */
    uint32_t dma_isr;
    uint32_t dma_ifcr;

    struct {
        uint32_t ccr;
        uint32_t cndtr;
        uint32_t cpar;
        uint32_t cmar;
    } dma_channels[7];   /* 7 DMA1 channels */

} Bus_Matrix_State;

void bus_matrix_init(Bus_Matrix_State *bus);
uint32_t bus_read(Bus_Matrix_State *bus, uint32_t addr);
void bus_write(Bus_Matrix_State *bus, uint32_t addr, uint32_t value);
uint8_t bus_is_accessible(Bus_Matrix_State *bus, uint32_t addr);

#endif /* BUS_MATRIX_H */
