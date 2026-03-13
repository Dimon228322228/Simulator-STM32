#ifndef EXECUTE_H
#define EXECUTE_H

#include "cpu_state.h"
#include "memory.h"
#include "gpio.h"
#include "tim6.h"
#include "nvic.h"

// Структура симулятора, объединяющая CPU и память
typedef struct {
    CPU_State cpu;
    Memory mem;
    GPIO_State gpio;
    TIM6_State tim6;
    NVIC_State nvic;
} Simulator;

// Выполнить одну инструкцию
void simulator_step(Simulator *sim);

// Запустить выполнение до остановки (пока не реализовано) или бесконечно
void simulator_run(Simulator *sim);

#endif // EXECUTE_H