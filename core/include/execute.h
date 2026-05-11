#ifndef EXECUTE_H
#define EXECUTE_H

#include "cpu_state.h"
#include "memory.h"
#include "gpio.h"
#include "tim6.h"
#include "nvic.h"
#include "bus_matrix.h"
#include "rcc.h"
#include "dma.h"
#include "usart.h"
#include "spi.h"
#include "i2c.h"
#include "cube_script.h"

/**
 * Simulator – top-level state aggregating CPU, memory, and all peripherals.
 */
typedef struct {
    CPU_State cpu;
    Memory    mem;
    GPIO_State gpio;
    TIM6_State tim6;
    NVIC_State nvic;
    Bus_Matrix_State bus;
    RCC_State rcc;
    DMA_State dma;
    USART_State usart1;
    USART_State usart2;
    USART_State usart3;
    SPI_State spi1;
    SPI_State spi2;
    SPI_State spi3;
    I2C_State i2c1;
    I2C_State i2c2;
    uint64_t tick_counter;
    uint8_t  cpu_in_isr;
    CubeScript script;
    int script_pc;
    uint32_t delay_counter;
    uint8_t script_active;
} Simulator;

/** Execute a single instruction cycle. */
void simulator_step(Simulator *sim);

/**
 * Run the simulator until PC leaves Flash range or an invalid instruction
 * is encountered.  Uses a configurable step limit to prevent infinite loops.
 */
void simulator_run(Simulator *sim, int max_steps);
void simulator_run_script_step(Simulator *sim);

#endif /* EXECUTE_H */