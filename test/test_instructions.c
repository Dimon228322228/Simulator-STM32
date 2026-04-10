/**
 * test_instructions.c - Unit tests for Thumb-16 instruction decoding
 *
 * Each test loads a single instruction into Flash, sets up registers,
 * executes one simulator_step(), and verifies the result.
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

/* ------------------------------------------------------------------ */
/*  Individual instruction tests                                       */
/* ------------------------------------------------------------------ */

static int test_mov_instruction(void) {
    printf("Testing MOV instruction...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);

    uint16_t program[] = {0x2005};  /* MOVS R0, #5 */
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;

    simulator_step(&sim);

    if (sim.cpu.regs[0] != 5) {
        printf("FAIL: Expected R0=5, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    printf("PASS: MOV instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

static int test_add_instruction(void) {
    printf("Testing ADD instruction...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);

    /* ADDS R0, R1, R2  →  R0 = R1 + R2 */
    uint16_t program[] = {0x1888};
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;
    sim.cpu.regs[1] = 10;
    sim.cpu.regs[2] = 15;

    simulator_step(&sim);

    if (sim.cpu.regs[0] != 25) {
        printf("FAIL: Expected R0=25, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    printf("PASS: ADD instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

static int test_sub_instruction(void) {
    printf("Testing SUB instruction...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);

    /* SUBS R0, R1, R2  →  R0 = R1 - R2 */
    uint16_t program[] = {0x1A88};
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;
    sim.cpu.regs[1] = 20;
    sim.cpu.regs[2] = 8;

    simulator_step(&sim);

    if (sim.cpu.regs[0] != 12) {
        printf("FAIL: Expected R0=12, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    printf("PASS: SUB instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

static int test_and_instruction(void) {
    printf("Testing AND instruction...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);

    /* ANDS R2, R0  →  R2 = R2 & R0 (both zero → 0) */
    uint16_t program[] = {0x400A};
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;

    simulator_step(&sim);

    if (sim.cpu.regs[2] != 0) {
        printf("FAIL: Expected R2=0, got R2=%u\n", sim.cpu.regs[2]);
        memory_free(&sim.mem);
        return 1;
    }
    printf("PASS: AND instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

static int test_branch_instruction(void) {
    printf("Testing B instruction...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);

    /* B .  (infinite loop to same address, offset = -1) */
    uint16_t program[] = {0xE7FF};
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;

    simulator_step(&sim);

    if (sim.cpu.pc != FLASH_BASE_ADDR) {
        printf("FAIL: Expected PC=0x%08X, got PC=0x%08X\n",
               FLASH_BASE_ADDR, sim.cpu.pc);
        memory_free(&sim.mem);
        return 1;
    }
    printf("PASS: B instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

static int test_ldr_instruction(void) {
    printf("Testing LDR instruction...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) { printf("FAIL: Memory init\n"); return 1; }
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);

    /* LDR R0, [R1, R2]  with R2=0 → word load from [R1] */
    uint16_t program[] = {0x6908};
    memcpy(sim.mem.flash, program, sizeof(program));

    /* Place test data in SRAM */
    sim.mem.sram[0] = 0x78;
    sim.mem.sram[1] = 0x56;
    sim.mem.sram[2] = 0x34;
    sim.mem.sram[3] = 0x12;

    sim.cpu.regs[1] = SRAM_BASE_ADDR;  /* base address */
    sim.cpu.regs[2] = 0;               /* offset */
    sim.cpu.pc = FLASH_BASE_ADDR;

    simulator_step(&sim);

    if (sim.cpu.regs[0] != 0x12345678) {
        printf("FAIL: Expected R0=0x12345678, got R0=0x%08X\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    printf("PASS: LDR instruction test\n");
    memory_free(&sim.mem);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int main(void) {
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
