#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "cpu_state.h"
#include "execute.h"
#include "gpio.h"
#include "tim6.h"
#include "nvic.h"

// Тесты для проверки работы системы команд

int test_mov_instruction() {
    printf("Testing MOV instruction...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);
    
    // Загружаем программу с MOV R0, #5
    uint16_t program[] = {0x2005}; // MOV R0, #5
    
    uint8_t *flash_ptr = sim.mem.flash;
    memcpy(flash_ptr, program, sizeof(program));
    
    // Устанавливаем PC на начало программы
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем одну инструкцию
    simulator_step(&sim);
    
    // Проверяем результат
    if (sim.cpu.regs[0] != 5) {
        printf("FAIL: MOV instruction test failed. Expected R0=5, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: MOV instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_add_instruction() {
    printf("Testing ADD instruction...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);
    
    // Загружаем программу с ADD R0, R1, R2
    uint16_t program[] = {0x1882}; // ADD R0, R1, R2 (R0 = R1 + R2)
    
    uint8_t *flash_ptr = sim.mem.flash;
    memcpy(flash_ptr, program, sizeof(program));
    
    // Устанавливаем значения регистров
    sim.cpu.regs[1] = 10;
    sim.cpu.regs[2] = 15;
    
    // Устанавливаем PC на начало программы
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем одну инструкцию
    simulator_step(&sim);
    
    // Проверяем результат
    if (sim.cpu.regs[0] != 25) {
        printf("FAIL: ADD instruction test failed. Expected R0=25, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: ADD instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_sub_instruction() {
    printf("Testing SUB instruction...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);
    
    // Загружаем программу с SUB R0, R1, R2
    uint16_t program[] = {0x1A82}; // SUB R0, R1, R2 (R0 = R1 - R2)
    
    uint8_t *flash_ptr = sim.mem.flash;
    memcpy(flash_ptr, program, sizeof(program));
    
    // Устанавливаем значения регистров
    sim.cpu.regs[1] = 20;
    sim.cpu.regs[2] = 8;
    
    // Устанавливаем PC на начало программы
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем одну инструкцию
    simulator_step(&sim);
    
    // Проверяем результат
    if (sim.cpu.regs[0] != 12) {
        printf("FAIL: SUB instruction test failed. Expected R0=12, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: SUB instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_and_instruction() {
    printf("Testing AND instruction...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);
    
    // Загружаем программу с AND R0, R1, R2
    uint16_t program[] = {0x4082}; // AND R0, R1, R2 (R0 = R1 & R2)
    
    uint8_t *flash_ptr = sim.mem.flash;
    memcpy(flash_ptr, program, sizeof(program));
    
    // Устанавливаем значения регистров
    sim.cpu.regs[1] = 0xF0F0;
    sim.cpu.regs[2] = 0x0F0F;
    
    // Устанавливаем PC на начало программы
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем одну инструкцию
    simulator_step(&sim);
    
    // Проверяем результат
    if (sim.cpu.regs[0] != 0) {
        printf("FAIL: AND instruction test failed. Expected R0=0, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: AND instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_branch_instruction() {
    printf("Testing B instruction...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);
    
    // Загружаем программу с B (бесконечный цикл)
    uint16_t program[] = {0xE7FE}; // B . (бесконечный цикл)
    
    uint8_t *flash_ptr = sim.mem.flash;
    memcpy(flash_ptr, program, sizeof(program));
    
    // Устанавливаем PC на начало программы
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем одну инструкцию
    simulator_step(&sim);
    
    // Проверяем, что PC изменился (должен указывать на адрес + 4)
    if (sim.cpu.pc != FLASH_BASE_ADDR + 4) {
        printf("FAIL: B instruction test failed. Expected PC=0x%08X, got PC=0x%08X\n", 
               FLASH_BASE_ADDR + 4, sim.cpu.pc);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: B instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_ldr_instruction() {
    printf("Testing LDR instruction...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);
    
    // Загружаем данные в память
    uint32_t test_data = 0x12345678;
    uint8_t *flash_ptr = sim.mem.flash;
    memcpy(flash_ptr, &test_data, sizeof(test_data));
    
    // Загружаем программу с LDR R0, [R1, #0]
    uint16_t program[] = {0x4801}; // LDR R0, [R1, #0] (загрузка слова)
    
    memcpy(flash_ptr + 2, program, sizeof(program));
    
    // Устанавливаем значение регистра R1
    sim.cpu.regs[1] = FLASH_BASE_ADDR;
    
    // Устанавливаем PC на начало программы
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем одну инструкцию
    simulator_step(&sim);
    
    // Проверяем результат
    if (sim.cpu.regs[0] != 0x12345678) {
        printf("FAIL: LDR instruction test failed. Expected R0=0x12345678, got R0=0x%08X\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: LDR instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

int main() {
    printf("Running instruction tests...\n\n");
    
    int failures = 0;
    
    failures += test_mov_instruction();
    failures += test_add_instruction();
    failures += test_sub_instruction();
    failures += test_and_instruction();
    failures += test_branch_instruction();
    failures += test_ldr_instruction();
    
    if (failures == 0) {
        printf("\nAll instruction tests passed!\n");
        return 0;
    } else {
        printf("\n%d instruction test(s) failed!\n", failures);
        return 1;
    }
}