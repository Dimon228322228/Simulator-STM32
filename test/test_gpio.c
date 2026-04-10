#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpio.h"
#include "memory_map.h"

// Тесты для GPIO периферии

int test_gpio_init() {
    printf("Testing GPIO initialization...\n");
    
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
    
    printf("PASS: GPIO initialization test\n");
    return 0;
}

int test_gpio_reset() {
    printf("Testing GPIO reset...\n");
    
    GPIO_State gpio;
    gpio_init(&gpio);
    
    // Меняем значения некоторых регистров
    gpio.ports[0].crl = 0x12345678;
    gpio.ports[0].odr = 0x87654321;
    
    // Сбрасываем GPIO
    gpio_reset(&gpio);
    
    // Проверяем, что все регистры обнулены
    if (gpio.ports[0].crl != 0 || gpio.ports[0].odr != 0) {
        printf("FAIL: GPIO reset did not clear registers\n");
        return 1;
    }
    
    printf("PASS: GPIO reset test\n");
    return 0;
}

int test_gpio_register_access() {
    printf("Testing GPIO register access...\n");
    
    GPIO_State gpio;
    gpio_init(&gpio);
    
    // Тестируем чтение регистров
    uint32_t addr_a_crl = GPIO_PORT_A_ADDR + GPIO_CRL_OFFSET;
    uint32_t addr_a_odr = GPIO_PORT_A_ADDR + GPIO_ODR_OFFSET;
    
    // Проверяем чтение несуществующего адреса
    uint32_t invalid_addr = 0xDEADBEEF;
    uint32_t result = gpio_read_register(&gpio, invalid_addr);
    if (result != 0xFFFFFFFF) {
        printf("FAIL: Reading from invalid address should return error value\n");
        return 1;
    }
    
    // Проверяем чтение существующих регистров
    uint32_t crl_result = gpio_read_register(&gpio, addr_a_crl);
    uint32_t odr_result = gpio_read_register(&gpio, addr_a_odr);
    
    if (crl_result != 0 || odr_result != 0) {
        printf("FAIL: Reading GPIO registers should return 0 initially\n");
        return 1;
    }
    
    // Проверяем запись регистров
    gpio_write_register(&gpio, addr_a_crl, 0x12345678);
    gpio_write_register(&gpio, addr_a_odr, 0x87654321);
    
    // Проверяем чтение после записи
    crl_result = gpio_read_register(&gpio, addr_a_crl);
    odr_result = gpio_read_register(&gpio, addr_a_odr);
    
    if (crl_result != 0x12345678 || odr_result != 0x87654321) {
        printf("FAIL: GPIO register write/read test failed\n");
        return 1;
    }
    
    printf("PASS: GPIO register access test\n");
    return 0;
}

int test_gpio_bsrr_write() {
    printf("Testing GPIO BSRR register write...\n");
    
    GPIO_State gpio;
    gpio_init(&gpio);
    
    uint32_t addr_a_bsrr = GPIO_PORT_A_ADDR + GPIO_BSRR_OFFSET;
    
    // Устанавливаем начальное значение ODR
    gpio.ports[0].odr = 0x00000000;
    
    // Записываем в BSRR: установить биты 0 и 2, сбросить биты 1 и 3
    // Биты 0-15 - сброс, биты 16-31 - установка
    gpio_write_register(&gpio, addr_a_bsrr, (0x0000000C | (0x00000005 << 16))); // Сброс битов 1,3, установка 0,2
    
    // Проверяем, что ODR обновился правильно
    if (gpio.ports[0].odr != 0x00000005) {
        printf("FAIL: BSRR write test failed\n");
        return 1;
    }
    
    printf("PASS: GPIO BSRR write test\n");
    return 0;
}

int main() {
    printf("Running GPIO tests...\n\n");
    
    int failures = 0;
    
    failures += test_gpio_init();
    failures += test_gpio_reset();
    failures += test_gpio_register_access();
    failures += test_gpio_bsrr_write();
    
    if (failures == 0) {
        printf("\nAll GPIO tests passed!\n");
        return 0;
    } else {
        printf("\n%d GPIO test(s) failed!\n", failures);
        return 1;
    }
}