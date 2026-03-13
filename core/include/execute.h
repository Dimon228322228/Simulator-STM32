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
} Simulator;

// Выполнить одну инструкцию
void simulator_step(Simulator *sim);

// Запустить выполнение до остановки (пока не реализовано) или бесконечно
void simulator_run(Simulator *sim);

#endif // EXECUTE_H