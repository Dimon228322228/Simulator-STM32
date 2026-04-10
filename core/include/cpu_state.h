#ifndef CPU_STATE_H
#define CPU_STATE_H

#include <stdint.h>

typedef struct {
    // Регистры общего назначения R0-R15
    uint32_t regs[16]; 
    
    // Специальные регистры
    uint32_t xpsr;      // Program Status Register
    uint32_t primask;
    uint32_t faultmask;
    uint32_t basepri;
    uint32_t control;
    
    // Указатели стека (MSP и PSP)
    uint32_t msp;
    uint32_t psp;
    
    // Счетчик команд
    uint32_t pc;
    
} CPU_State;

void cpu_reset(CPU_State *cpu);

#endif // CPU_STATE_H