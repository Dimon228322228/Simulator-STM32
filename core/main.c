#include <stdio.h>
#include "memory.h"
#include "cpu_state.h"
#include "execute.h"
#include <string.h>

int main() {
    Simulator sim;

    printf("Initializing Simulator...\n");

    if (!memory_init(&sim.mem)) {
        return 1;
    }
    
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    nvic_extended_init(&sim.nvic_ext);
    bus_matrix_init(&sim.bus);
    rcc_init(&sim.rcc);
    dma_init(&sim.dma);
    
    // Инициализация периферии
    usart_init(&sim.usart1, USART1_BASE_ADDR);
    usart_init(&sim.usart2, USART2_BASE_ADDR);
    usart_init(&sim.usart3, USART3_BASE_ADDR);
    spi_init(&sim.spi1, SPI1_BASE_ADDR);
    spi_init(&sim.spi2, SPI2_BASE_ADDR);
    spi_init(&sim.spi3, SPI3_BASE_ADDR);
    i2c_init(&sim.i2c1, I2C1_BASE_ADDR);
    i2c_init(&sim.i2c2, I2C2_BASE_ADDR);
    
    cpu_reset(&sim.cpu);
    
    // Устанавливаем начальный вектор сброса (Reset Handler)
    // В реальном МК по адресу 0x08000000 лежит адрес стека, а по 0x08000004 - адрес PC.
    // Для простоты нашего симулятора мы сразу начинаем выполнение с 0x08000000.
    sim.cpu.pc = FLASH_BASE_ADDR;

    // "Прошиваем" программу вручную в Flash
    // Коды инструкций (Little Endian):
    // MOV R0, #5   -> 00100 000 00000101 -> 0x2005
    // MOV R1, #3   -> 00100 001 00000011 -> 0x2103
    // ADD R2, R0, R1 -> 0001100 001 000 010 -> 0x1882
    // B . (offset 0) -> 11100 00000000000 -> 0xE000 (Loop to self relative to PC+4)
    //    Поскольку PC увеличивается на 2 перед исполнением, нам нужен переход на -2 относительно текущего PC+4.
    //    Формула: Target = PC + 4 + (offset*2).
    //    Target = Current + 2. Current + 4 + off*2 = Current + 2 => off*2 = -2 => off = -1.
    //    Binary for -1 (11 bits): 11111111111 -> 0x7FF.
    //    Instr: 11100 11111111111 -> 0xE7FF (Это классический B .)
    
    uint16_t program[] = {
        0x2005, // MOV R0, #5
        0x2103, // MOV R1, #3
        0x1882, // ADD R2, R0, R1
        0xE7FE  // B . (бесконечный цикл)
    };

    // Копируем в память симулятора
    uint8_t *flash_ptr = sim.mem.flash;
    memcpy(flash_ptr, program, sizeof(program));

    printf("Starting simulation...\n\n");
    simulator_run(&sim);

    printf("\nSimulation finished.\n");
    printf("Final R0: 0x%08X\n", sim.cpu.regs[0]);
    printf("Final R1: 0x%08X\n", sim.cpu.regs[1]);
    printf("Final R2: 0x%08X\n", sim.cpu.regs[2]); // Ожидается 8
    
    memory_free(&sim.mem);
    return 0;
}