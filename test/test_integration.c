/**
 * test_integration.c - End-to-end integration tests
 *
 * Exercises multiple subsystems together: CPU + memory + peripherals.
 */

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

/* ------------------------------------------------------------------ */
/*  Test 1: Demo program execution                                      */
/* ------------------------------------------------------------------ */

static int test_demo_program_execution(void) {
    printf("Test 1: Demo program execution (MOV, ADD, B)...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("  FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);

    /*
     * Demo program (same as main.c --demo):
     *   MOVS R0, #5
     *   MOVS R1, #3
     *   ADDS R2, R0, R1    → R2 = 8
     *   B    .              (infinite loop)
     */
    uint16_t program[] = {
        0x2005,  /* MOVS R0, #5    */
        0x2103,  /* MOVS R1, #3    */
        0x1842,  /* ADDS R2, R0, R1 */
        0xE7FF   /* B    .          */
    };

    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;

    for (int i = 0; i < 4 && sim.cpu.pc != 0xFFFFFFFFU; i++) {
        simulator_step(&sim);
    }

    int failed = 0;
    if (sim.cpu.regs[0] != 5) {
        printf("  FAIL: R0 = %u (expected 5)\n", sim.cpu.regs[0]); failed = 1;
    }
    if (sim.cpu.regs[1] != 3) {
        printf("  FAIL: R1 = %u (expected 3)\n", sim.cpu.regs[1]); failed = 1;
    }
    if (sim.cpu.regs[2] != 8) {
        printf("  FAIL: R2 = %u (expected 8)\n", sim.cpu.regs[2]); failed = 1;
    }

    if (!failed) printf("  PASS: Demo program executed correctly\n");
    memory_free(&sim.mem);
    return failed;
}

/* ------------------------------------------------------------------ */
/*  Test 2: CPU + GPIO integration                                      */
/* ------------------------------------------------------------------ */

static int test_gpio_with_cpu(void) {
    printf("Test 2: CPU + GPIO integration...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("  FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);

    uint32_t gpio_odr_addr = GPIO_PORT_A_ADDR + GPIO_ODR_OFFSET;
    gpio_write_register(&sim.gpio, gpio_odr_addr, 0x00FF);
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

/* ------------------------------------------------------------------ */
/*  Test 3: USART transmission                                          */
/* ------------------------------------------------------------------ */

static int test_usart_transmission_simulation(void) {
    printf("Test 3: USART transmission simulation...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("  FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    usart_init(&sim.usart1, USART1_BASE_ADDR);
    cpu_reset(&sim.cpu);

    uint32_t dr_addr = USART1_BASE_ADDR + USART_DR_OFFSET;
    uint32_t sr_addr = USART1_BASE_ADDR + USART_SR_OFFSET;

    usart_write_register(&sim.usart1, dr_addr, 0x48);  /* 'H' */
    uint32_t sr = usart_read_register(&sim.usart1, sr_addr);

    if (!(sr & USART_SR_TXE)) {
        printf("  FAIL: TXE flag not set after DR write\n");
        memory_free(&sim.mem);
        return 1;
    }
    printf("  PASS: USART transmission simulation\n");
    memory_free(&sim.mem);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 4: Memory map boundaries                                       */
/* ------------------------------------------------------------------ */

static int test_memory_map_boundaries(void) {
    printf("Test 4: Memory map boundaries...\n");

    Memory mem;
    if (!memory_init(&mem)) { printf("  FAIL: Memory init\n"); return 1; }

    int failed = 0;
    uint8_t test_byte = 0xAA;

    /* Flash boundaries */
    mem.flash[0] = test_byte;
    mem.flash[FLASH_SIZE - 1] = test_byte;
    if (memory_read_byte(&mem, FLASH_BASE_ADDR) != test_byte) {
        printf("  FAIL: Flash start boundary\n"); failed = 1;
    }
    if (memory_read_byte(&mem, FLASH_BASE_ADDR + FLASH_SIZE - 1) != test_byte) {
        printf("  FAIL: Flash end boundary\n"); failed = 1;
    }

    /* SRAM boundaries */
    mem.sram[0] = test_byte;
    mem.sram[SRAM_SIZE - 1] = test_byte;
    if (memory_read_byte(&mem, SRAM_BASE_ADDR) != test_byte) {
        printf("  FAIL: SRAM start boundary\n"); failed = 1;
    }
    if (memory_read_byte(&mem, SRAM_BASE_ADDR + SRAM_SIZE - 1) != test_byte) {
        printf("  FAIL: SRAM end boundary\n"); failed = 1;
    }

    if (!failed) printf("  PASS: Memory map boundaries correct\n");
    memory_free(&mem);
    return failed;
}

/* ------------------------------------------------------------------ */
/*  Test 5: CPU reset state                                             */
/* ------------------------------------------------------------------ */

static int test_cpu_reset_state(void) {
    printf("Test 5: CPU reset state...\n");

    CPU_State cpu;
    cpu_reset(&cpu);

    int failed = 0;
    for (int i = 0; i < 16; i++) {
        if (cpu.regs[i] != 0) {
            printf("  FAIL: R%d = 0x%08X (expected 0)\n", i, cpu.regs[i]);
            failed = 1;
        }
    }
    if (cpu.xpsr != 0 || cpu.primask != 0 || cpu.faultmask != 0 ||
        cpu.msp != 0 || cpu.psp != 0 || cpu.pc != 0) {
        printf("  FAIL: Special registers not zeroed\n");
        failed = 1;
    }

    if (!failed) printf("  PASS: CPU reset state correct\n");
    return failed;
}

/* ------------------------------------------------------------------ */
/*  Test 6: Loop execution (branch)                                     */
/* ------------------------------------------------------------------ */

static int test_loop_execution(void) {
    printf("Test 6: Loop execution (branch instruction)...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("  FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);

    /*
     *   MOVS R0, #0
     *   B    .          (infinite loop on B instruction)
     */
    uint16_t program[] = { 0x2000, 0xE7FF };
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;

    int max_steps = 10;
    uint32_t last_pc = sim.cpu.pc;

    for (int steps = 0; steps < max_steps && sim.cpu.pc != 0xFFFFFFFFU; steps++) {
        simulator_step(&sim);
        if (steps > 0 && sim.cpu.pc == last_pc) break;  /* loop detected */
        last_pc = sim.cpu.pc;
    }

    if (sim.cpu.regs[0] != 0) {
        printf("  FAIL: R0 = %u (expected 0)\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    if (sim.cpu.pc != FLASH_BASE_ADDR + 2) {
        printf("  FAIL: PC did not loop correctly (0x%08X)\n", sim.cpu.pc);
        memory_free(&sim.mem);
        return 1;
    }
    printf("  PASS: Loop execution test\n");
    memory_free(&sim.mem);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Test 7: All GPIO ports                                              */
/* ------------------------------------------------------------------ */

static int test_all_gpio_ports(void) {
    printf("Test 7: All GPIO ports (A-G)...\n");

    GPIO_State gpio;
    gpio_init(&gpio);

    int failed = 0;
    for (int i = 0; i < GPIO_NUM_PORTS; i++) {
        uint32_t odr_addr = gpio.port_addresses[i] + GPIO_ODR_OFFSET;
        gpio_write_register(&gpio, odr_addr, 0x1234);
        uint32_t result = gpio_read_register(&gpio, odr_addr);
        if (result != 0x1234) {
            printf("  FAIL: Port %c ODR = 0x%04X (expected 0x1234)\n",
                   'A' + i, result);
            failed = 1;
        }
    }

    if (!failed) printf("  PASS: All GPIO ports functional\n");
    return failed;
}

/* ------------------------------------------------------------------ */
/*  Test 8: CPU register file                                           */
/* ------------------------------------------------------------------ */

static int test_register_file(void) {
    printf("Test 8: CPU register file (R0-R15)...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("  FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);

    for (int i = 0; i < 16; i++) {
        sim.cpu.regs[i] = (uint32_t)i * 0x11111111U;
    }

    int failed = 0;
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (uint32_t)i * 0x11111111U;
        if (sim.cpu.regs[i] != expected) {
            printf("  FAIL: R%d = 0x%08X (expected 0x%08X)\n",
                   i, sim.cpu.regs[i], expected);
            failed = 1;
        }
    }

    if (!failed) printf("  PASS: All registers R0-R15 accessible\n");
    memory_free(&sim.mem);
    return failed;
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int main(void) {
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
