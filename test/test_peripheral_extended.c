/**
 * test_peripheral_extended.c - Peripheral device unit tests
 *
 * Covers TIM6, USART, NVIC, GPIO output operations, and memory access.
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
/*  TIM6 tests                                                          */
/* ------------------------------------------------------------------ */

static int test_tim6_initialization(void) {
    printf("Testing TIM6 initialization...\n");

    TIM6_State tim6;
    tim6_init(&tim6);

    if (tim6.regs.cr1 != 0 || tim6.regs.cr2 != 0 ||
        tim6.regs.sr != 0 || tim6.regs.cnt != 0) {
        printf("FAIL: TIM6 registers not zero-initialized\n");
        return 1;
    }
    printf("  PASS: TIM6 initialization\n");
    return 0;
}

static int test_tim6_register_access(void) {
    printf("Testing TIM6 register read/write...\n");

    TIM6_State tim6;
    tim6_init(&tim6);

    uint32_t cr1_addr = TIM6_BASE_ADDR + TIM6_CR1_OFFSET;
    uint32_t cnt_addr = TIM6_BASE_ADDR + TIM6_CNT_OFFSET;
    uint32_t arr_addr = TIM6_BASE_ADDR + TIM6_ARR_OFFSET;

    tim6_write_register(&tim6, cr1_addr, 0x0001);
    if (tim6_read_register(&tim6, cr1_addr) != 0x0001) {
        printf("FAIL: CR1 write/read\n"); return 1;
    }
    printf("  PASS: CR1 register access\n");

    tim6_write_register(&tim6, cnt_addr, 0x00FF);
    if (tim6_read_register(&tim6, cnt_addr) != 0x00FF) {
        printf("FAIL: CNT write/read\n"); return 1;
    }
    printf("  PASS: CNT register access\n");

    tim6_write_register(&tim6, arr_addr, 0xFFFF);
    if (tim6_read_register(&tim6, arr_addr) != 0xFFFF) {
        printf("FAIL: ARR write/read\n"); return 1;
    }
    printf("  PASS: ARR register access\n");

    return 0;
}

static int test_tim6_control_bits(void) {
    printf("Testing TIM6 control bits...\n");

    TIM6_State tim6;
    tim6_init(&tim6);
    uint32_t cr1_addr = TIM6_BASE_ADDR + TIM6_CR1_OFFSET;

    tim6_write_register(&tim6, cr1_addr, TIM6_CR1_CEN);
    if (!(tim6_read_register(&tim6, cr1_addr) & TIM6_CR1_CEN)) {
        printf("FAIL: CEN bit\n"); return 1;
    }
    printf("  PASS: CEN bit\n");

    tim6_write_register(&tim6, cr1_addr, TIM6_CR1_UDIS);
    if (!(tim6_read_register(&tim6, cr1_addr) & TIM6_CR1_UDIS)) {
        printf("FAIL: UDIS bit\n"); return 1;
    }
    printf("  PASS: UDIS bit\n");

    return 0;
}

/* ------------------------------------------------------------------ */
/*  USART tests                                                         */
/* ------------------------------------------------------------------ */

static int test_usart_initialization(void) {
    printf("Testing USART initialization...\n");

    USART_State usart;
    usart_init(&usart, USART1_BASE_ADDR);

    if (usart.regs.cr1 != 0) {
        printf("FAIL: USART CR1 not zero-initialized\n");
        return 1;
    }
    printf("  PASS: USART initialization\n");
    return 0;
}

static int test_usart_register_access(void) {
    printf("Testing USART register read/write...\n");

    USART_State usart;
    usart_init(&usart, USART1_BASE_ADDR);

    uint32_t cr1_addr = USART1_BASE_ADDR + USART_CR1_OFFSET;
    uint32_t dr_addr  = USART1_BASE_ADDR + USART_DR_OFFSET;
    uint32_t sr_addr  = USART1_BASE_ADDR + USART_SR_OFFSET;

    usart_write_register(&usart, cr1_addr, USART_CR1_TE | USART_CR1_RE);
    if (usart_read_register(&usart, cr1_addr) != (USART_CR1_TE | USART_CR1_RE)) {
        printf("FAIL: CR1 write/read\n"); return 1;
    }
    printf("  PASS: CR1 register access\n");

    usart_write_register(&usart, dr_addr, 0x41);  /* 'A' */
    if (!(usart_read_register(&usart, sr_addr) & USART_SR_TXE)) {
        printf("FAIL: TXE flag not set after DR write\n"); return 1;
    }
    printf("  PASS: DR write sets TXE flag\n");

    return 0;
}

static int test_usart_transmit(void) {
    printf("Testing USART transmission...\n");

    USART_State usart;
    usart_init(&usart, USART1_BASE_ADDR);

    uint32_t dr_addr = USART1_BASE_ADDR + USART_DR_OFFSET;
    uint32_t sr_addr = USART1_BASE_ADDR + USART_SR_OFFSET;

    usart_write_register(&usart, dr_addr, 0x55);

    uint32_t sr = usart_read_register(&usart, sr_addr);
    if (!(sr & USART_SR_TXE)) {
        printf("FAIL: TXE flag not set\n");
        return 1;
    }
    printf("  PASS: USART transmission\n");
    return 0;
}

/* ------------------------------------------------------------------ */
/*  NVIC tests                                                          */
/* ------------------------------------------------------------------ */

static int test_nvic_initialization(void) {
    printf("Testing NVIC initialization...\n");

    NVIC_State nvic;
    nvic_init(&nvic);

    for (int i = 0; i < NVIC_NUM_INTERRUPTS / 32; i++) {
        if (nvic.iser[i] != 0 || nvic.ispr[i] != 0 ||
            nvic.icer[i] != 0 || nvic.icpr[i] != 0) {
            printf("FAIL: NVIC register %d not zero-initialized\n", i);
            return 1;
        }
    }
    printf("  PASS: NVIC initialization\n");
    return 0;
}

static int test_nvic_enable_disable(void) {
    printf("Testing NVIC enable/disable interrupts...\n");

    NVIC_State nvic;
    nvic_init(&nvic);
    uint8_t irq = 5;

    nvic_enable_interrupt(&nvic, irq);
    if (!nvic_is_enabled(&nvic, irq)) {
        printf("FAIL: Interrupt %d not enabled\n", irq); return 1;
    }
    printf("  PASS: Enable interrupt\n");

    nvic_disable_interrupt(&nvic, irq);
    if (nvic_is_enabled(&nvic, irq)) {
        printf("FAIL: Interrupt %d still enabled\n", irq); return 1;
    }
    printf("  PASS: Disable interrupt\n");

    return 0;
}

static int test_nvic_pending(void) {
    printf("Testing NVIC pending interrupts...\n");

    NVIC_State nvic;
    nvic_init(&nvic);
    uint8_t irq = 7;

    nvic_set_pending(&nvic, irq);
    if (!nvic_is_pending(&nvic, irq)) {
        printf("FAIL: Interrupt %d not pending\n", irq); return 1;
    }
    printf("  PASS: Set pending interrupt\n");

    nvic_clear_pending(&nvic, irq);
    if (nvic_is_pending(&nvic, irq)) {
        printf("FAIL: Interrupt %d still pending\n", irq); return 1;
    }
    printf("  PASS: Clear pending interrupt\n");

    return 0;
}

/* ------------------------------------------------------------------ */
/*  GPIO output tests                                                   */
/* ------------------------------------------------------------------ */

static int test_gpio_output(void) {
    printf("Testing GPIO output operations...\n");

    GPIO_State gpio;
    gpio_init(&gpio);

    uint32_t odr_addr  = GPIO_PORT_A_ADDR + GPIO_ODR_OFFSET;
    uint32_t bsrr_addr = GPIO_PORT_A_ADDR + GPIO_BSRR_OFFSET;

    gpio_write_register(&gpio, odr_addr, 0x00FF);
    if (gpio_read_register(&gpio, odr_addr) != 0x00FF) {
        printf("FAIL: ODR write/read\n"); return 1;
    }
    printf("  PASS: ODR write/read\n");

    /* Set bit 0 via BSRR (upper 16 bits) */
    gpio_write_register(&gpio, bsrr_addr, 0x00010000);
    if (!(gpio_read_register(&gpio, odr_addr) & 0x0001)) {
        printf("FAIL: BSRR set bit\n"); return 1;
    }
    printf("  PASS: BSRR set bits\n");

    /* Reset bit 0 via BSRR (lower 16 bits) */
    gpio_write_register(&gpio, bsrr_addr, 0x0001);
    if (gpio_read_register(&gpio, odr_addr) & 0x0001) {
        printf("FAIL: BSRR reset bit\n"); return 1;
    }
    printf("  PASS: BSRR reset bits\n");

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Memory access tests                                                 */
/* ------------------------------------------------------------------ */

static int test_memory_access(void) {
    printf("Testing memory access (Flash/SRAM)...\n");

    Memory mem;
    if (!memory_init(&mem)) { printf("FAIL: Memory init\n"); return 1; }

    /* Flash byte */
    mem.flash[0] = 0x01;
    if (memory_read_byte(&mem, FLASH_BASE_ADDR) != 0x01) {
        printf("FAIL: Flash byte read\n"); memory_free(&mem); return 1;
    }
    printf("  PASS: Flash byte access\n");

    /* SRAM byte */
    mem.sram[0] = 0xAB;
    if (memory_read_byte(&mem, SRAM_BASE_ADDR) != 0xAB) {
        printf("FAIL: SRAM byte read\n"); memory_free(&mem); return 1;
    }
    printf("  PASS: SRAM byte access\n");

    /* SRAM halfword (little-endian) */
    mem.sram[0] = 0xAB; mem.sram[1] = 0xCD;
    if (memory_read_halfword(&mem, SRAM_BASE_ADDR) != 0xCDAB) {
        printf("FAIL: SRAM halfword read\n"); memory_free(&mem); return 1;
    }
    printf("  PASS: SRAM halfword access (little-endian)\n");

    /* SRAM word (little-endian) */
    mem.sram[0] = 0xAB; mem.sram[1] = 0xCD;
    mem.sram[2] = 0xEF; mem.sram[3] = 0x00;
    if (memory_read_word(&mem, SRAM_BASE_ADDR) != 0x00EFCDAB) {
        printf("FAIL: SRAM word read\n"); memory_free(&mem); return 1;
    }
    printf("  PASS: SRAM word access (little-endian)\n");

    memory_free(&mem);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("========================================\n");
    printf("  Peripheral Device Tests\n");
    printf("========================================\n\n");

    int failures = 0;
    failures += test_tim6_initialization();
    failures += test_tim6_register_access();
    failures += test_tim6_control_bits();
    failures += test_usart_initialization();
    failures += test_usart_register_access();
    failures += test_usart_transmit();
    failures += test_nvic_initialization();
    failures += test_nvic_enable_disable();
    failures += test_nvic_pending();
    failures += test_gpio_output();
    failures += test_memory_access();

    printf("\n========================================\n");
    if (failures == 0) {
        printf("  All peripheral tests PASSED!\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("  %d test(s) FAILED!\n", failures);
        printf("========================================\n");
        return 1;
    }
}
