#ifndef EXECUTE_H
#define EXECUTE_H

#include "cpu_state.h"
#include "memory.h"

#include "tim6.h"
#include "nvic.h"
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

// Выполнить одну инструкцию
void simulator_step(Simulator *sim);

// Запустить выполнение до остановки (пока не реализовано) или бесконечно
void simulator_run(Simulator *sim);

#endif // EXECUTE_H
