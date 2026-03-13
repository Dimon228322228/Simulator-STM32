#ifndef BUS_MATRIX_H
#define BUS_MATRIX_H

#include <stdint.h>
#include "memory_map.h"

// Шина AHB (Advanced High-performance Bus)
#define AHB_BASE_ADDR           0x40000000U
#define AHB_SIZE                0x40000000U  // 1GB

// Шина APB1 (Advanced Peripheral Bus 1)
#define APB1_BASE_ADDR          0x40000000U
#define APB1_SIZE               0x00010000U  // 64KB

// Шина APB2 (Advanced Peripheral Bus 2)
#define APB2_BASE_ADDR          0x40010000U
#define APB2_SIZE               0x00010000U  // 64KB

// Адреса периферии
#define GPIOA_BASE_ADDR         0x40010800U
#define GPIOB_BASE_ADDR         0x40010C00U
#define GPIOC_BASE_ADDR         0x40011000U
#define GPIOD_BASE_ADDR         0x40011400U
#define GPIOE_BASE_ADDR         0x40011800U
#define GPIOF_BASE_ADDR         0x40011C00U
#define GPIOG_BASE_ADDR         0x40012000U

#define TIM6_BASE_ADDR          0x40001400U
#define TIM7_BASE_ADDR          0x40001800U

// RCC (Reset and Clock Control)
#define RCC_BASE_ADDR           0x40021000U
#define RCC_CR_OFFSET           0x00
#define RCC_CFGR_OFFSET         0x04
#define RCC_CIR_OFFSET          0x08
#define RCC_APB2RSTR_OFFSET     0x0C
#define RCC_APB1RSTR_OFFSET     0x10
#define RCC_AHBENR_OFFSET       0x14
#define RCC_APB2ENR_OFFSET      0x18
#define RCC_APB1ENR_OFFSET      0x1C
#define RCC_BDCR_OFFSET         0x20
#define RCC_CSR_OFFSET          0x24

// DMA (Direct Memory Access)
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

// Bus Matrix State Structure
typedef struct {
    uint32_t ahb_base;
    uint32_t apb1_base;
    uint32_t apb2_base;
    
    // Состояние тактирования
    uint32_t rcc_cr;
    uint32_t rcc_cfgr;
    uint32_t rcc_apb2rstr;
    uint32_t rcc_apb1rstr;
    uint32_t rcc_ahbenr;
    uint32_t rcc_apb2enr;
    uint32_t rcc_apb1enr;
    
    // DMA состояние
    uint32_t dma_isr;
    uint32_t dma_ifcr;
    
    // Состояние каналов DMA
    struct {
        uint32_t ccr;
        uint32_t cndtr;
        uint32_t cpar;
        uint32_t cmar;
    } dma_channels[7];  // 7 каналов DMA1
    
} Bus_Matrix_State;

// Инициализация шины
void bus_matrix_init(Bus_Matrix_State *bus);

// Чтение из шины
uint32_t bus_read(Bus_Matrix_State *bus, uint32_t addr);

// Запись в шину
void bus_write(Bus_Matrix_State *bus, uint32_t addr, uint32_t value);

// Проверка доступности адреса
uint8_t bus_is_accessible(Bus_Matrix_State *bus, uint32_t addr);

#endif // BUS_MATRIX_H