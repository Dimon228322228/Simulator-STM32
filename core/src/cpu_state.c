#include "cpu_state.h"
#include "memory_map.h"

void cpu_reset(CPU_State *cpu) {
    for (int i = 0; i < 16; i++) {
        cpu->regs[i] = 0;
    }
    cpu->xpsr = 0;
    cpu->primask = 0;
    cpu->faultmask = 0;
    cpu->basepri = 0;
    cpu->control = 0;
    
    // Согласно архитектуре Cortex-M3, начальное значение SP берется из 
    // первых 4 байт Flash (по адресу 0x08000000), а PC - из следующих 4 байт.
    // Это будет реализовано в функции загрузки, здесь просто зануляем.
    cpu->msp = 0;
    cpu->psp = 0;
    cpu->pc = 0;
}