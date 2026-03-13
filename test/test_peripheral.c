#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "cpu_state.h"
#include "execute.h"
#include "gpio.h"
#include "tim6.h"
#include "nvic.h"
#include "bus_matrix.h"
#include "rcc.h"
#include "dma.h"

// Тесты для проверки работы внешних устройств

int test_gpio_peripheral() {
    printf("Testing GPIO peripheral...\n");
    
    GPIO_State gpio;
    gpio_init(&gpio);
    
    // Проверяем, что все порты инициализированы нулями
    for (int i = 0; i < GPIO_NUM_PORTS; i++) {
        if (gpio.ports[i].crl != 0 || gpio.ports[i].crh != 0 ||
            gpio.ports[i].idr != 0 || gpio.ports[i].odr != 0 ||
            gpio.ports[i].bsrr != 0 || gpio.ports[i].brr != 0 ||
            gpio.ports[i].lckr != 0) {
            printf("FAIL: GPIO port %d not initialized correctly\n", i);
            return 1;
        }
    }
    
    // Проверяем чтение регистров
    uint32_t addr_a_crl = GPIO_PORT_A_ADDR + GPIO_CRL_OFFSET;
    uint32_t result = gpio_read_register(&gpio, addr_a_crl);
    if (result != 0) {
        printf("FAIL: GPIO register read test failed\n");
        return 1;
    }
    
    // Проверяем запись регистров
    gpio_write_register(&gpio, addr_a_crl, 0x12345678);
    result = gpio_read_register(&gpio, addr_a_crl);
    if (result != 0x12345678) {
        printf("FAIL: GPIO register write test failed\n");
        return 1;
    }
    
    printf("PASS: GPIO peripheral test\n");
    return 0;
}

int test_tim6_peripheral() {
    printf("Testing TIM6 peripheral...\n");
    
    TIM6_State tim6;
    tim6_init(&tim6);
    
    // Проверяем, что все регистры инициализированы нулями
    if (tim6.regs.cr1 != 0 || tim6.regs.cr2 != 0 || tim6.regs.smcr != 0 ||
        tim6.regs.dier != 0 || tim6.regs.sr != 0 || tim6.regs.egr != 0 ||
        tim6.regs.cnt != 0 || tim6.regs.psc != 0 || tim6.regs.arr != 0) {
        printf("FAIL: TIM6 registers not initialized correctly\n");
        return 1;
    }
    
    // Проверяем чтение регистров
    uint32_t addr = TIM6_BASE_ADDR + TIM6_CR1_OFFSET;
    uint32_t result = tim6_read_register(&tim6, addr);
    if (result != 0) {
        printf("FAIL: TIM6 register read test failed\n");
        return 1;
    }
    
    // Проверяем запись регистров
    tim6_write_register(&tim6, addr, 0x12345678);
    result = tim6_read_register(&tim6, addr);
    if (result != 0x12345678) {
        printf("FAIL: TIM6 register write test failed\n");
        return 1;
    }
    
    printf("PASS: TIM6 peripheral test\n");
    return 0;
}

int test_bus_matrix() {
    printf("Testing Bus Matrix...\n");
    
    Bus_Matrix_State bus;
    bus_matrix_init(&bus);
    
    // Проверяем базовые адреса
    if (bus.ahb_base != AHB_BASE_ADDR) {
        printf("FAIL: Bus matrix AHB base address incorrect\n");
        return 1;
    }
    
    if (bus.apb1_base != APB1_BASE_ADDR) {
        printf("FAIL: Bus matrix APB1 base address incorrect\n");
        return 1;
    }
    
    if (bus.apb2_base != APB2_BASE_ADDR) {
        printf("FAIL: Bus matrix APB2 base address incorrect\n");
        return 1;
    }
    
    // Проверяем доступность адресов
    if (!bus_is_accessible(&bus, FLASH_BASE_ADDR)) {
        printf("FAIL: Bus matrix accessibility test failed\n");
        return 1;
    }
    
    if (!bus_is_accessible(&bus, SRAM_BASE_ADDR)) {
        printf("FAIL: Bus matrix accessibility test failed\n");
        return 1;
    }
    
    printf("PASS: Bus Matrix test\n");
    return 0;
}

int test_rcc_peripheral() {
    printf("Testing RCC peripheral...\n");
    
    RCC_State rcc;
    rcc_init(&rcc);
    
    // Проверяем, что регистры инициализированы правильно
    if (rcc.cr != (RCC_CR_HSIRDY)) {
        printf("FAIL: RCC CR register not initialized correctly\n");
        return 1;
    }
    
    // Проверяем чтение регистров
    uint32_t addr = RCC_BASE_ADDR + RCC_CR_OFFSET;
    uint32_t result = rcc_read_register(&rcc, addr);
    if (result != RCC_CR_HSIRDY) {
        printf("FAIL: RCC register read test failed\n");
        return 1;
    }
    
    // Проверяем запись регистров
    rcc_write_register(&rcc, addr, 0x12345678);
    result = rcc_read_register(&rcc, addr);
    if (result != 0x12345678) {
        printf("FAIL: RCC register write test failed\n");
        return 1;
    }
    
    printf("PASS: RCC peripheral test\n");
    return 0;
}

int test_dma_peripheral() {
    printf("Testing DMA peripheral...\n");
    
    DMA_State dma;
    dma_init(&dma);
    
    // Проверяем, что все регистры инициализированы нулями
    if (dma.isr != 0 || dma.ifcr != 0) {
        printf("FAIL: DMA ISR/IFCR registers not initialized correctly\n");
        return 1;
    }
    
    // Проверяем каналы
    for (int i = 0; i < 7; i++) {
        if (dma.channels[i].ccr != 0 || dma.channels[i].cndtr != 0 ||
            dma.channels[i].cpar != 0 || dma.channels[i].cmar != 0) {
            printf("FAIL: DMA channel %d registers not initialized correctly\n", i);
            return 1;
        }
    }
    
    // Проверяем чтение регистров
    uint32_t addr = DMA1_BASE_ADDR + DMA_ISR_OFFSET;
    uint32_t result = dma_read_register(&dma, addr);
    if (result != 0) {
        printf("FAIL: DMA register read test failed\n");
        return 1;
    }
    
    // Проверяем запись регистров
    dma_write_register(&dma, addr, 0x12345678);
    result = dma_read_register(&dma, addr);
    if (result != 0) { // ISR только для чтения
        printf("FAIL: DMA register write test failed\n");
        return 1;
    }
    
    printf("PASS: DMA peripheral test\n");
    return 0;
}

int main() {
    printf("Running peripheral tests...\n\n");
    
    int failures = 0;
    
    failures += test_gpio_peripheral();
    failures += test_tim6_peripheral();
    failures += test_bus_matrix();
    failures += test_rcc_peripheral();
    failures += test_dma_peripheral();
    
    if (failures == 0) {
        printf("\nAll peripheral tests passed!\n");
        return 0;
    } else {
        printf("\n%d peripheral test(s) failed!\n", failures);
        return 1;
    }
}