#ifndef DMA_H
#define DMA_H

#include <stdint.h>

// DMA Register Addresses
#define DMA1_BASE_ADDR          0x40020000U
#define DMA1_Channel1_BASE_ADDR 0x40020008U
#define DMA1_Channel2_BASE_ADDR 0x4002001C
#define DMA1_Channel3_BASE_ADDR 0x40020030
#define DMA1_Channel4_BASE_ADDR 0x40020044
#define DMA1_Channel5_BASE_ADDR 0x40020058
#define DMA1_Channel6_BASE_ADDR 0x4002006C
#define DMA1_Channel7_BASE_ADDR 0x40020080

// DMA Registers
#define DMA_ISR_OFFSET          0x00
#define DMA_IFCR_OFFSET         0x04
#define DMA_CCR1_OFFSET         0x08
#define DMA_CNDTR1_OFFSET       0x0C
#define DMA_CPAR1_OFFSET        0x10
#define DMA_CMAR1_OFFSET        0x14

// DMA Channel Structure
typedef struct {
    uint32_t ccr;       // Channel Control Register
    uint32_t cndtr;     // Channel Number of Data Register
    uint32_t cpar;      // Channel Peripheral Address Register
    uint32_t cmar;      // Channel Memory Address Register
} DMA_Channel;

// DMA State Structure
typedef struct {
    uint32_t isr;       // Interrupt Status Register
    uint32_t ifcr;      // Interrupt Flag Clear Register
    DMA_Channel channels[7];  // 7 каналов DMA1
} DMA_State;

// Initialize DMA state
void dma_init(DMA_State *dma);

// Reset DMA state
void dma_reset(DMA_State *dma);

// Read DMA register
uint32_t dma_read_register(DMA_State *dma, uint32_t addr);

// Write DMA register
void dma_write_register(DMA_State *dma, uint32_t addr, uint32_t value);

// Start DMA transfer
void dma_start_transfer(DMA_State *dma, int channel);

// Check if DMA transfer is complete
uint8_t dma_is_complete(DMA_State *dma, int channel);

#endif // DMA_H