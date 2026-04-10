#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "cpu_state.h"
#include "execute.h"

// Тесты для основного симулятора

int test_simulator_basic() {
    printf("Testing basic simulator functionality...\n");
    
    Simulator sim;
    
    // Инициализация памяти
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    // Инициализация GPIO
    gpio_init(&sim.gpio);
    
    // Инициализация CPU
    cpu_reset(&sim.cpu);
    
    // Проверяем, что CPU инициализирован правильно
    if (sim.cpu.pc != 0) {
        printf("FAIL: CPU PC not reset correctly\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    // Проверяем, что все регистры R0-R15 равны 0
    for (int i = 0; i < 16; i++) {
        if (sim.cpu.regs[i] != 0) {
            printf("FAIL: CPU register R%d not reset correctly\n", i);
            memory_free(&sim.mem);
            return 1;
        }
    }
    
    // Проверяем специальные регистры
    if (sim.cpu.xpsr != 0 || sim.cpu.primask != 0 || 
        sim.cpu.faultmask != 0 || sim.cpu.basepri != 0 || 
        sim.cpu.control != 0 || sim.cpu.msp != 0 || sim.cpu.psp != 0) {
        printf("FAIL: Special CPU registers not reset correctly\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: Basic simulator test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_memory_access() {
    printf("Testing memory access...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    // Тестируем чтение/запись в Flash
    uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78};
    uint8_t *flash_ptr = sim.mem.flash;
    
    // Записываем данные в Flash
    memcpy(flash_ptr, test_data, sizeof(test_data));
    
    // Читаем данные обратно
    uint8_t read_data[4];
    memcpy(read_data, flash_ptr, sizeof(read_data));
    
    if (memcmp(test_data, read_data, sizeof(test_data)) != 0) {
        printf("FAIL: Flash memory read/write test failed\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    // Тестируем чтение/запись в SRAM
    uint8_t sram_test_data[] = {0xAB, 0xCD, 0xEF, 0x01};
    uint8_t *sram_ptr = sim.mem.sram;
    
    // Записываем данные в SRAM
    memcpy(sram_ptr, sram_test_data, sizeof(sram_test_data));
    
    // Читаем данные обратно
    uint8_t sram_read_data[4];
    memcpy(sram_read_data, sram_ptr, sizeof(sram_read_data));
    
    if (memcmp(sram_test_data, sram_read_data, sizeof(sram_test_data)) != 0) {
        printf("FAIL: SRAM memory read/write test failed\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: Memory access test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_instruction_execution() {
    printf("Testing instruction execution...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);
    
    // Загружаем простую программу: MOV R0, #5
    uint16_t program[] = {0x2005}; // MOV R0, #5
    
    // Копируем в память симулятора
    uint8_t *flash_ptr = sim.mem.flash;
    memcpy(flash_ptr, program, sizeof(program));
    
    // Устанавливаем PC на начало программы
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем одну инструкцию
    simulator_step(&sim);
    
    // Проверяем результат
    if (sim.cpu.regs[0] != 5) {
        printf("FAIL: Instruction execution test failed. Expected R0=5, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: Instruction execution test\n");
    memory_free(&sim.mem);
    return 0;
}

int main() {
    printf("Running simulator tests...\n\n");
    
    int failures = 0;
    
    failures += test_simulator_basic();
    failures += test_memory_access();
    failures += test_instruction_execution();
    
    if (failures == 0) {
        printf("\nAll simulator tests passed!\n");
        return 0;
    } else {
        printf("\n%d simulator test(s) failed!\n", failures);
        return 1;
    }
}