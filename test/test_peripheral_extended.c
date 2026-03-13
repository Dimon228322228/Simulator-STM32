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
// Тесты периферийных устройств
// ============================================

int test_tim6_initialization() {
    printf("Testing TIM6 initialization...\n");
    
    TIM6_State tim6;
    tim6_init(&tim6);
    
    // Проверяем, что все регистры инициализированы нулями
    if (tim6.regs.cr1 != 0 || tim6.regs.cr2 != 0 || 
        tim6.regs.sr != 0 || tim6.regs.cnt != 0) {
        printf("FAIL: TIM6 registers not zero-initialized\n");
        return 1;
    }
    
    printf("  PASS: TIM6 initialization\n");
    return 0;
}

int test_tim6_register_access() {
    printf("Testing TIM6 register read/write...\n");
    
    TIM6_State tim6;
    tim6_init(&tim6);
    
    uint32_t addr_cr1 = TIM6_BASE_ADDR + TIM6_CR1_OFFSET;
    uint32_t addr_cnt = TIM6_BASE_ADDR + TIM6_CNT_OFFSET;
    uint32_t addr_arr = TIM6_BASE_ADDR + TIM6_ARR_OFFSET;
    
    // Тест записи/чтения CR1
    tim6_write_register(&tim6, addr_cr1, 0x0001); // CEN = 1
    uint32_t result = tim6_read_register(&tim6, addr_cr1);
    if (result != 0x0001) {
        printf("FAIL: CR1 write/read test\n");
        return 1;
    }
    printf("  PASS: CR1 register access\n");
    
    // Тест записи/чтения CNT
    tim6_write_register(&tim6, addr_cnt, 0x00FF);
    result = tim6_read_register(&tim6, addr_cnt);
    if (result != 0x00FF) {
        printf("FAIL: CNT write/read test\n");
        return 1;
    }
    printf("  PASS: CNT register access\n");
    
    // Тест записи/чтения ARR
    tim6_write_register(&tim6, addr_arr, 0xFFFF);
    result = tim6_read_register(&tim6, addr_arr);
    if (result != 0xFFFF) {
        printf("FAIL: ARR write/read test\n");
        return 1;
    }
    printf("  PASS: ARR register access\n");
    
    return 0;
}

int test_tim6_control_bits() {
    printf("Testing TIM6 control bits...\n");
    
    TIM6_State tim6;
    tim6_init(&tim6);
    
    uint32_t addr_cr1 = TIM6_BASE_ADDR + TIM6_CR1_OFFSET;
    
    // Тест бита CEN (Counter Enable)
    tim6_write_register(&tim6, addr_cr1, TIM6_CR1_CEN);
    uint32_t result = tim6_read_register(&tim6, addr_cr1);
    if (!(result & TIM6_CR1_CEN)) {
        printf("FAIL: CEN bit test\n");
        return 1;
    }
    printf("  PASS: CEN bit\n");
    
    // Тест бита UDIS (Update Disable)
    tim6_write_register(&tim6, addr_cr1, TIM6_CR1_UDIS);
    result = tim6_read_register(&tim6, addr_cr1);
    if (!(result & TIM6_CR1_UDIS)) {
        printf("FAIL: UDIS bit test\n");
        return 1;
    }
    printf("  PASS: UDIS bit\n");
    
    return 0;
}

int test_usart_initialization() {
    printf("Testing USART initialization...\n");
    
    USART_State usart;
    usart_init(&usart, USART1_BASE_ADDR);
    
    // Проверяем инициализацию
    if (usart.regs.cr1 != 0 || usart.regs.sr != 0) {
        printf("FAIL: USART registers not zero-initialized\n");
        return 1;
    }
    
    printf("  PASS: USART initialization\n");
    return 0;
}

int test_usart_register_access() {
    printf("Testing USART register read/write...\n");
    
    USART_State usart;
    usart_init(&usart, USART1_BASE_ADDR);
    
    uint32_t addr_sr = USART1_BASE_ADDR + USART_SR_OFFSET;
    uint32_t addr_dr = USART1_BASE_ADDR + USART_DR_OFFSET;
    uint32_t addr_cr1 = USART1_BASE_ADDR + USART_CR1_OFFSET;
    
    // Тест записи/чтения CR1
    usart_write_register(&usart, addr_cr1, USART_CR1_TE | USART_CR1_RE);
    uint32_t result = usart_read_register(&usart, addr_cr1);
    if (result != (USART_CR1_TE | USART_CR1_RE)) {
        printf("FAIL: CR1 write/read test. Expected 0x%08X, got 0x%08X\n",
               USART_CR1_TE | USART_CR1_RE, result);
        return 1;
    }
    printf("  PASS: CR1 register access\n");
    
    // Тест записи в DR (передача)
    usart_write_register(&usart, addr_dr, 0x41); // 'A'
    result = usart_read_register(&usart, addr_sr);
    if (!(result & USART_SR_TXE)) {
        printf("FAIL: TXE flag not set after DR write\n");
        return 1;
    }
    printf("  PASS: DR write sets TXE flag\n");
    
    return 0;
}

int test_usart_transmit() {
    printf("Testing USART transmission...\n");
    
    USART_State usart;
    usart_init(&usart, USART1_BASE_ADDR);
    
    uint32_t addr_dr = USART1_BASE_ADDR + USART_DR_OFFSET;
    uint32_t addr_sr = USART1_BASE_ADDR + USART_SR_OFFSET;
    
    // Передаём байт
    usart_write_register(&usart, addr_dr, 0x55);
    
    // Проверяем флаг TXE
    uint32_t sr = usart_read_register(&usart, addr_sr);
    if (!(sr & USART_SR_TXE)) {
        printf("FAIL: TXE flag not set\n");
        return 1;
    }
    
    // Проверяем флаг TC
    if (!(sr & USART_SR_TC)) {
        printf("FAIL: TC flag not set\n");
        return 1;
    }
    
    printf("  PASS: USART transmission\n");
    return 0;
}

int test_nvic_initialization() {
    printf("Testing NVIC initialization...\n");
    
    NVIC_State nvic;
    nvic_init(&nvic);
    
    // Проверяем, что все регистры инициализированы нулями
    for (int i = 0; i < NVIC_NUM_INTERRUPTS/32; i++) {
        if (nvic.iser[i] != 0 || nvic.ispr[i] != 0 || 
            nvic.icer[i] != 0 || nvic.icpr[i] != 0) {
            printf("FAIL: NVIC register %d not zero-initialized\n", i);
            return 1;
        }
    }
    
    printf("  PASS: NVIC initialization\n");
    return 0;
}

int test_nvic_enable_disable() {
    printf("Testing NVIC enable/disable interrupts...\n");
    
    NVIC_State nvic;
    nvic_init(&nvic);
    
    uint8_t irq = 5;
    
    // Включаем прерывание
    nvic_enable_interrupt(&nvic, irq);
    
    // Проверяем, что прерывание включено
    if (!nvic_is_enabled(&nvic, irq)) {
        printf("FAIL: Interrupt %d not enabled\n", irq);
        return 1;
    }
    printf("  PASS: Enable interrupt\n");
    
    // Выключаем прерывание
    nvic_disable_interrupt(&nvic, irq);
    
    // Проверяем, что прерывание выключено
    if (nvic_is_enabled(&nvic, irq)) {
        printf("FAIL: Interrupt %d still enabled after disable\n", irq);
        return 1;
    }
    printf("  PASS: Disable interrupt\n");
    
    return 0;
}

int test_nvic_pending() {
    printf("Testing NVIC pending interrupts...\n");
    
    NVIC_State nvic;
    nvic_init(&nvic);
    
    uint8_t irq = 7;
    
    // Устанавливаем pending
    nvic_set_pending(&nvic, irq);
    
    // Проверяем pending
    if (!nvic_is_pending(&nvic, irq)) {
        printf("FAIL: Interrupt %d not pending\n", irq);
        return 1;
    }
    printf("  PASS: Set pending interrupt\n");
    
    // Очищаем pending
    nvic_clear_pending(&nvic, irq);
    
    // Проверяем, что pending очищен
    if (nvic_is_pending(&nvic, irq)) {
        printf("FAIL: Interrupt %d still pending after clear\n", irq);
        return 1;
    }
    printf("  PASS: Clear pending interrupt\n");
    
    return 0;
}

int test_gpio_output() {
    printf("Testing GPIO output operations...\n");
    
    GPIO_State gpio;
    gpio_init(&gpio);
    
    uint32_t addr_odr = GPIO_PORT_A_ADDR + GPIO_ODR_OFFSET;
    uint32_t addr_bsrr = GPIO_PORT_A_ADDR + GPIO_BSRR_OFFSET;
    
    // Тест записи в ODR
    gpio_write_register(&gpio, addr_odr, 0x00FF);
    uint32_t result = gpio_read_register(&gpio, addr_odr);
    if (result != 0x00FF) {
        printf("FAIL: ODR write/read test\n");
        return 1;
    }
    printf("  PASS: ODR write/read\n");
    
    // Тест BSRR (установка битов)
    gpio_write_register(&gpio, addr_bsrr, 0x00010000); // Установить бит 0
    result = gpio_read_register(&gpio, addr_odr);
    if (!(result & 0x0001)) {
        printf("FAIL: BSRR set bit test\n");
        return 1;
    }
    printf("  PASS: BSRR set bits\n");
    
    // Тест BSRR (сброс битов)
    gpio_write_register(&gpio, addr_bsrr, 0x0001); // Сбросить бит 0
    result = gpio_read_register(&gpio, addr_odr);
    if (result & 0x0001) {
        printf("FAIL: BSRR reset bit test\n");
        return 1;
    }
    printf("  PASS: BSRR reset bits\n");
    
    return 0;
}

int test_memory_access() {
    printf("Testing memory access (Flash/SRAM)...\n");
    
    Memory mem;
    if (!memory_init(&mem)) {
        printf("FAIL: Memory initialization\n");
        return 1;
    }
    
    // Тест записи/чтения Flash
    uint8_t flash_data[] = {0x01, 0x23, 0x45, 0x67};
    memcpy(mem.flash, flash_data, sizeof(flash_data));
    
    if (memory_read_byte(&mem, FLASH_BASE_ADDR) != 0x01) {
        printf("FAIL: Flash byte read\n");
        memory_free(&mem);
        return 1;
    }
    printf("  PASS: Flash byte access\n");
    
    // Тест записи/чтения SRAM
    uint8_t sram_data[] = {0xAB, 0xCD, 0xEF};
    memcpy(mem.sram, sram_data, sizeof(sram_data));
    
    if (memory_read_byte(&mem, SRAM_BASE_ADDR) != 0xAB) {
        printf("FAIL: SRAM byte read\n");
        memory_free(&mem);
        return 1;
    }
    printf("  PASS: SRAM byte access\n");
    
    // Тест halfword
    uint16_t hw = memory_read_halfword(&mem, SRAM_BASE_ADDR);
    if (hw != 0xCDAB) { // Little-endian
        printf("FAIL: SRAM halfword read. Expected 0xCDAB, got 0x%04X\n", hw);
        memory_free(&mem);
        return 1;
    }
    printf("  PASS: SRAM halfword access (little-endian)\n");
    
    // Тест word
    uint32_t w = memory_read_word(&mem, SRAM_BASE_ADDR);
    if (w != 0x00EFCDAB) { // Little-endian
        printf("FAIL: SRAM word read. Expected 0x00EFCDAB, got 0x%08X\n", w);
        memory_free(&mem);
        return 1;
    }
    printf("  PASS: SRAM word access (little-endian)\n");
    
    memory_free(&mem);
    return 0;
}

int main() {
    printf("========================================\n");
    printf("  Peripheral Device Tests\n");
    printf("========================================\n\n");
    
    int failures = 0;
    
    // TIM6 тесты
    failures += test_tim6_initialization();
    failures += test_tim6_register_access();
    failures += test_tim6_control_bits();
    
    // USART тесты
    failures += test_usart_initialization();
    failures += test_usart_register_access();
    failures += test_usart_transmit();
    
    // NVIC тесты
    failures += test_nvic_initialization();
    failures += test_nvic_enable_disable();
    failures += test_nvic_pending();
    
    // GPIO тесты
    failures += test_gpio_output();
    
    // Память
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
