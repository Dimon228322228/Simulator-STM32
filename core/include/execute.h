#ifndef EXECUTE_H
#define EXECUTE_H

#include "cpu_state.h"
#include "memory.h"
#include "gpio.h"
#include "tim6.h"
#include "nvic.h"
#include "nvic_extended.h"
#include "bus_matrix.h"
#include "rcc.h"
#include "dma.h"
#include "usart.h"
#include "spi.h"
#include "i2c.h"

// Структура симулятора, объединяющая CPU и память
typedef struct {
    CPU_State cpu;
    Memory mem;
    GPIO_State gpio;
    TIM6_State tim6;
    NVIC_State nvic;
    NVIC_Extended_State nvic_ext;
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
} Simulator;

// Функции для работы с флагами
void update_flags(CPU_State *cpu, uint32_t result, uint32_t carry_in);
void update_arithmetic_flags(CPU_State *cpu, uint32_t a, uint32_t b, uint32_t result, uint8_t is_subtract);

// Функции для работы с условными инструкциями
uint8_t check_condition(uint16_t instr, CPU_State *cpu);

#endif // EXECUTE_H

// Выполнить одну инструкцию
void simulator_step(Simulator *sim);

// Запустить выполнение до остановки (пока не реализовано) или бесконечно
void simulator_run(Simulator *sim);

#endif // EXECUTE_H