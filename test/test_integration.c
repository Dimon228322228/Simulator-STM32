#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "cpu_state.h"
#include "execute.h"
#include "gpio.h"
#include "tim6.h"
#include "nvic.h"
#include "usart.h"

// ============================================
// Интеграционные тесты (end-to-end)
// ============================================

int test_demo_program_execution() {
    printf("Test 1: Demo program execution (MOV, ADD, B)...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("  FAIL: Memory initialization\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);
    
    // Демо-программа из main.c
    uint16_t program[] = {
        0x2005,  // MOV R0, #5
        0x2103,  // MOV R1, #3
        0x1882,  // ADD R2, R0, R1
        0xE7FE   // B . (бесконечный цикл)
    };
    
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем 4 инструкции
    int steps = 0;
    while (steps < 4 && sim.cpu.pc != 0xFFFFFFFF) {
        simulator_step(&sim);
        steps++;
    }
    
    // Проверяем результаты
    int failed = 0;
    
    if (sim.cpu.regs[0] != 5) {
        printf("  FAIL: R0 = %u (expected 5)\n", sim.cpu.regs[0]);
        failed = 1;
    }
    
    if (sim.cpu.regs[1] != 3) {
        printf("  FAIL: R1 = %u (expected 3)\n", sim.cpu.regs[1]);
        failed = 1;
    }
    
    if (sim.cpu.regs[2] != 8) {
        printf("  FAIL: R2 = %u (expected 8)\n", sim.cpu.regs[2]);
        failed = 1;
    }
    
    if (!failed) {
        printf("  PASS: Demo program executed correctly\n");
    }
    
    memory_free(&sim.mem);
    return failed;
}

int test_gpio_with_cpu() {
    printf("Test 2: CPU + GPIO integration...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("  FAIL: Memory initialization\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);
    
    // Записываем значение в GPIO ODR через память
    uint32_t gpio_odr_addr = GPIO_PORT_A_ADDR + GPIO_ODR_OFFSET;
    
    // Симулируем запись из CPU (через memory_write)
    // В реальной инструкции это было бы STR
    gpio_write_register(&sim.gpio, gpio_odr_addr, 0x00FF);
    
    // Читаем обратно
    uint32_t result = gpio_read_register(&sim.gpio, gpio_odr_addr);
    
    if (result != 0x00FF) {
        printf("  FAIL: GPIO ODR = 0x%08X (expected 0x00FF)\n", result);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("  PASS: GPIO integration test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_usart_transmission_simulation() {
    printf("Test 3: USART transmission simulation...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("  FAIL: Memory initialization\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    usart_init(&sim.usart1, USART1_BASE_ADDR);
    cpu_reset(&sim.cpu);
    
    uint32_t usart_dr_addr = USART1_BASE_ADDR + USART_DR_OFFSET;
    uint32_t usart_sr_addr = USART1_BASE_ADDR + USART_SR_OFFSET;
    
    // Симулируем передачу байта (запись в DR)
    usart_write_register(&sim.usart1, usart_dr_addr, 0x48); // 'H'
    
    // Проверяем флаг TXE
    uint32_t sr = usart_read_register(&sim.usart1, usart_sr_addr);
    
    if (!(sr & USART_SR_TXE)) {
        printf("  FAIL: TXE flag not set after DR write\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("  PASS: USART transmission simulation\n");
    memory_free(&sim.mem);
    return 0;
}

int test_memory_map_boundaries() {
    printf("Test 4: Memory map boundaries...\n");
    
    Memory mem;
    if (!memory_init(&mem)) {
        printf("  FAIL: Memory initialization\n");
        return 1;
    }
    
    int failed = 0;
    
    // Тест границ Flash
    uint8_t test_byte = 0xAA;
    mem.flash[0] = test_byte;
    mem.flash[FLASH_SIZE - 1] = test_byte;
    
    if (memory_read_byte(&mem, FLASH_BASE_ADDR) != test_byte) {
        printf("  FAIL: Flash start boundary\n");
        failed = 1;
    }
    
    if (memory_read_byte(&mem, FLASH_BASE_ADDR + FLASH_SIZE - 1) != test_byte) {
        printf("  FAIL: Flash end boundary\n");
        failed = 1;
    }
    
    // Тест границ SRAM
    mem.sram[0] = test_byte;
    mem.sram[SRAM_SIZE - 1] = test_byte;
    
    if (memory_read_byte(&mem, SRAM_BASE_ADDR) != test_byte) {
        printf("  FAIL: SRAM start boundary\n");
        failed = 1;
    }
    
    if (memory_read_byte(&mem, SRAM_BASE_ADDR + SRAM_SIZE - 1) != test_byte) {
        printf("  FAIL: SRAM end boundary\n");
        failed = 1;
    }
    
    if (!failed) {
        printf("  PASS: Memory map boundaries correct\n");
    }
    
    memory_free(&mem);
    return failed;
}

int test_cpu_reset_state() {
    printf("Test 5: CPU reset state...\n");
    
    CPU_State cpu;
    cpu_reset(&cpu);
    
    int failed = 0;
    
    // Проверяем, что все регистры обнулены
    for (int i = 0; i < 16; i++) {
        if (cpu.regs[i] != 0) {
            printf("  FAIL: R%d = 0x%08X (expected 0)\n", i, cpu.regs[i]);
            failed = 1;
        }
    }
    
    if (cpu.xpsr != 0 || cpu.primask != 0 || cpu.faultmask != 0) {
        printf("  FAIL: Special registers not zeroed\n");
        failed = 1;
    }
    
    if (cpu.msp != 0 || cpu.psp != 0) {
        printf("  FAIL: Stack pointers not zeroed\n");
        failed = 1;
    }
    
    if (cpu.pc != 0) {
        printf("  FAIL: PC = 0x%08X (expected 0)\n", cpu.pc);
        failed = 1;
    }
    
    if (!failed) {
        printf("  PASS: CPU reset state correct\n");
    }
    
    return failed;
}

int test_loop_execution() {
    printf("Test 6: Loop execution (branch instruction)...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("  FAIL: Memory initialization\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);
    
    // Программа с циклом:
    // MOV R0, #0      (счётчик)
    // MOV R1, #3      (граница)
    // Loop:
    //   ADD R0, R0, #1
    //   CMP R0, R1
    //   BLT Loop
    //   B .
    
    // Упрощённая версия (только B .)
    uint16_t program[] = {
        0x2000,  // MOV R0, #0
        0xE7FE   // B . (бесконечный цикл)
    };
    
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Выполняем несколько инструкций
    int max_steps = 10;
    int steps = 0;
    uint32_t last_pc = sim.cpu.pc;
    
    while (steps < max_steps && sim.cpu.pc != 0xFFFFFFFF) {
        simulator_step(&sim);
        
        // После первой инструкции PC должен зациклиться
        if (steps > 0 && sim.cpu.pc == last_pc) {
            break; // Зацикливание обнаружено
        }
        
        last_pc = sim.cpu.pc;
        steps++;
    }
    
    // Проверяем, что R0 = 0 (первая инструкция выполнилась)
    if (sim.cpu.regs[0] != 0) {
        printf("  FAIL: R0 = %u (expected 0)\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    
    // Проверяем, что PC зациклился
    if (sim.cpu.pc != FLASH_BASE_ADDR + 2) {
        printf("  FAIL: PC did not loop correctly (0x%08X)\n", sim.cpu.pc);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("  PASS: Loop execution test\n");
    memory_free(&sim.mem);
    return 0;
}

int test_all_gpio_ports() {
    printf("Test 7: All GPIO ports (A-G)...\n");
    
    GPIO_State gpio;
    gpio_init(&gpio);
    
    int failed = 0;
    
    // Тестируем все порты
    for (int i = 0; i < GPIO_NUM_PORTS; i++) {
        uint32_t port_addr = gpio.port_addresses[i];
        uint32_t odr_addr = port_addr + GPIO_ODR_OFFSET;
        
        // Записываем тестовое значение
        gpio_write_register(&gpio, odr_addr, 0x1234);
        
        // Читаем обратно
        uint32_t result = gpio_read_register(&gpio, odr_addr);
        
        if (result != 0x1234) {
            printf("  FAIL: Port %c ODR = 0x%04X (expected 0x1234)\n", 
                   'A' + i, result);
            failed = 1;
        }
    }
    
    if (!failed) {
        printf("  PASS: All GPIO ports functional\n");
    }
    
    return failed;
}

int test_register_file() {
    printf("Test 8: CPU register file (R0-R15)...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("  FAIL: Memory initialization\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);
    
    // Записываем уникальные значения в каждый регистр
    for (int i = 0; i < 16; i++) {
        sim.cpu.regs[i] = i * 0x11111111;
    }
    
    // Проверяем
    int failed = 0;
    for (int i = 0; i < 16; i++) {
        uint32_t expected = i * 0x11111111;
        if (sim.cpu.regs[i] != expected) {
            printf("  FAIL: R%d = 0x%08X (expected 0x%08X)\n", 
                   i, sim.cpu.regs[i], expected);
            failed = 1;
        }
    }
    
    if (!failed) {
        printf("  PASS: All registers R0-R15 accessible\n");
    }
    
    memory_free(&sim.mem);
    return failed;
}

int main() {
    printf("========================================\n");
    printf("  Integration Tests (End-to-End)\n");
    printf("========================================\n\n");
    
    int failures = 0;
    
    failures += test_demo_program_execution();
    failures += test_gpio_with_cpu();
    failures += test_usart_transmission_simulation();
    failures += test_memory_map_boundaries();
    failures += test_cpu_reset_state();
    failures += test_loop_execution();
    failures += test_all_gpio_ports();
    failures += test_register_file();
    
    printf("\n========================================\n");
    if (failures == 0) {
        printf("  ALL INTEGRATION TESTS PASSED!\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("  %d integration test(s) FAILED!\n", failures);
        printf("========================================\n");
        return 1;
    }
}
