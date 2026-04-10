#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "memory_map.h"

// GPIO Port Addresses
#define GPIO_PORT_A_ADDR 0x40010800U
#define GPIO_PORT_B_ADDR 0x40010C00U
#define GPIO_PORT_C_ADDR 0x40011000U
#define GPIO_PORT_D_ADDR 0x40011400U
#define GPIO_PORT_E_ADDR 0x40011800U
#define GPIO_PORT_F_ADDR 0x40011C00U
#define GPIO_PORT_G_ADDR 0x40012000U

// GPIO Register Offsets
#define GPIO_CRL_OFFSET   0x00
#define GPIO_CRH_OFFSET   0x04
#define GPIO_IDR_OFFSET   0x08
#define GPIO_ODR_OFFSET   0x0C
#define GPIO_BSRR_OFFSET  0x10
#define GPIO_BRR_OFFSET   0x14
#define GPIO_LCKR_OFFSET  0x18

// Number of GPIO ports
#define GPIO_NUM_PORTS 7

// GPIO Port Structure
typedef struct {
    uint32_t crl;      // Configuration Register Low
    uint32_t crh;      // Configuration Register High
    uint32_t idr;      // Input Data Register
    uint32_t odr;      // Output Data Register
    uint32_t bsrr;     // Bit Set/Reset Register
    uint32_t brr;      // Bit Reset Register
    uint32_t lckr;     // Lock Key Register
} GPIO_Port;

// GPIO State Structure
typedef struct {
    GPIO_Port ports[GPIO_NUM_PORTS];  // Array of GPIO ports
    uint32_t port_addresses[GPIO_NUM_PORTS];  // Base addresses of ports
} GPIO_State;

// Initialize GPIO state
void gpio_init(GPIO_State *gpio);

// Reset GPIO state
void gpio_reset(GPIO_State *gpio);

// Read GPIO register
uint32_t gpio_read_register(GPIO_State *gpio, uint32_t addr);

// Write GPIO register
void gpio_write_register(GPIO_State *gpio, uint32_t addr, uint32_t value);

#endif // GPIO_H