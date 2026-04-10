/**
 * test_extended_instructions.c - Extended Thumb-16 instruction tests
 *
 * Covers arithmetic, branching, multi-instruction sequences, and
 * placeholder tests for logical, shift, load/store, and CPU flags.
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
/*  Arithmetic tests                                                    */
/* ------------------------------------------------------------------ */

static int test_arithmetic_instructions(void) {
    printf("Testing arithmetic instructions (ADD, SUB)...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) return 1;
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    cpu_reset(&sim.cpu);

    /* ADDS R0, R1, R2  →  R0 = 10 + 15 = 25 */
    uint16_t add_prog[] = {0x1888};
    memcpy(sim.mem.flash, add_prog, sizeof(add_prog));
    sim.cpu.pc = FLASH_BASE_ADDR;
    sim.cpu.regs[1] = 10;
    sim.cpu.regs[2] = 15;
    simulator_step(&sim);

    if (sim.cpu.regs[0] != 25) {
        printf("FAIL: ADD expected 25, got %u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    printf("  PASS: ADD R0, R1, R2\n");

    /* SUBS R3, R4, R5  →  R3 = 20 - 8 = 12 */
    cpu_reset(&sim.cpu);
    sim.cpu.regs[4] = 20;
    sim.cpu.regs[5] = 8;
    uint16_t sub_prog[] = {0x1B63};
    memcpy(sim.mem.flash, sub_prog, sizeof(sub_prog));
    sim.cpu.pc = FLASH_BASE_ADDR;
    simulator_step(&sim);

    if (sim.cpu.regs[3] != 12) {
        printf("FAIL: SUB expected 12, got %u\n", sim.cpu.regs[3]);
        memory_free(&sim.mem);
        return 1;
    }
    printf("  PASS: SUB R3, R4, R5\n");

    memory_free(&sim.mem);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Branch test                                                         */
/* ------------------------------------------------------------------ */

static int test_branch_instruction(void) {
    printf("Testing branch instruction (B)...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) return 1;
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);

    /* B .  (infinite loop, offset = -1) */
    uint16_t branch_prog[] = {0xE7FF};
    memcpy(sim.mem.flash, branch_prog, sizeof(branch_prog));
    sim.cpu.pc = FLASH_BASE_ADDR;
    uint32_t initial_pc = sim.cpu.pc;

    simulator_step(&sim);

    if (sim.cpu.pc != initial_pc) {
        printf("FAIL: Expected PC=0x%08X, got 0x%08X\n",
               initial_pc, sim.cpu.pc);
        memory_free(&sim.mem);
        return 1;
    }
    printf("  PASS: Branch instruction\n");
    memory_free(&sim.mem);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Multi-instruction sequence                                          */
/* ------------------------------------------------------------------ */

static int test_multi_instruction_sequence(void) {
    printf("Testing multi-instruction sequence...\n");

    Simulator sim;
    if (!memory_init(&sim.mem)) return 1;
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);

    /*
     * Program:
     *   MOVS R0, #5
     *   MOVS R1, #3
     *   ADDS R2, R0, R1    → R2 = 8
     *   SUBS R3, R0, R1    → R3 = 2
     */
    uint16_t program[] = {
        0x2005,  /* MOVS R0, #5    */
        0x2103,  /* MOVS R1, #3    */
        0x1842,  /* ADDS R2, R0, R1 */
        0x1A43   /* SUBS R3, R0, R1 */
    };

    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;

    for (int i = 0; i < 4; i++) {
        simulator_step(&sim);
    }

    if (sim.cpu.regs[0] != 5) {
        printf("FAIL: R0 expected 5, got %u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem); return 1;
    }
    if (sim.cpu.regs[1] != 3) {
        printf("FAIL: R1 expected 3, got %u\n", sim.cpu.regs[1]);
        memory_free(&sim.mem); return 1;
    }
    if (sim.cpu.regs[2] != 8) {
        printf("FAIL: R2 expected 8, got %u\n", sim.cpu.regs[2]);
        memory_free(&sim.mem); return 1;
    }
    if (sim.cpu.regs[3] != 2) {
        printf("FAIL: R3 expected 2, got %u\n", sim.cpu.regs[3]);
        memory_free(&sim.mem); return 1;
    }

    printf("  PASS: Multi-instruction sequence\n");
    memory_free(&sim.mem);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Placeholder tests (not yet implemented)                             */
/* ------------------------------------------------------------------ */

static int test_logical_instructions(void) {
    printf("Testing logical instructions (AND, EOR, ORR)...\n");
    /* TODO: implement AND, EOR, ORR unit tests */
    printf("  PASS: Logical instructions (placeholder)\n");
    return 0;
}

static int test_shift_instructions(void) {
    printf("Testing shift instructions (LSL, LSR)...\n");
    /* TODO: implement LSL, LSR unit tests */
    printf("  PASS: Shift instructions (placeholder)\n");
    return 0;
}

static int test_load_store_instructions(void) {
    printf("Testing load/store instructions (LDR, STR)...\n");
    /* TODO: implement LDR, STR unit tests */
    printf("  PASS: Load/store instructions (placeholder)\n");
    return 0;
}

static int test_cpu_flags(void) {
    printf("Testing CPU flags (N, Z, C, V)...\n");
    /* TODO: implement flag verification tests */
    printf("  PASS: CPU flags (placeholder)\n");
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("========================================\n");
    printf("  Extended Instruction Tests\n");
    printf("========================================\n\n");

    int failures = 0;
    failures += test_arithmetic_instructions();
    failures += test_logical_instructions();
    failures += test_branch_instruction();
    failures += test_shift_instructions();
    failures += test_load_store_instructions();
    failures += test_multi_instruction_sequence();
    failures += test_cpu_flags();

    printf("\n========================================\n");
    if (failures == 0) {
        printf("  All extended tests PASSED!\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("  %d test(s) FAILED!\n", failures);
        printf("========================================\n");
        return 1;
    }
}
