#include "gpio.h"
#include <stdio.h>
#include <string.h>

// GPIO port base addresses
static const uint32_t gpio_port_addresses[GPIO_NUM_PORTS] = {
    GPIO_PORT_A_ADDR,
    GPIO_PORT_B_ADDR,
    GPIO_PORT_C_ADDR,
    GPIO_PORT_D_ADDR,
    GPIO_PORT_E_ADDR,
    GPIO_PORT_F_ADDR,
    GPIO_PORT_G_ADDR
};

void gpio_init(GPIO_State *gpio) {
    // Initialize all ports to zero
    memset(gpio, 0, sizeof(GPIO_State));
    
    // Set port addresses
    for (int i = 0; i < GPIO_NUM_PORTS; i++) {
        gpio->port_addresses[i] = gpio_port_addresses[i];
    }
}

void gpio_reset(GPIO_State *gpio) {
    // Reset all GPIO ports
    for (int i = 0; i < GPIO_NUM_PORTS; i++) {
        gpio->ports[i].crl = 0;
        gpio->ports[i].crh = 0;
        gpio->ports[i].idr = 0;
        gpio->ports[i].odr = 0;
        gpio->ports[i].bsrr = 0;
        gpio->ports[i].brr = 0;
        gpio->ports[i].lckr = 0;
    }
}

// Helper function to get port index from address
static int get_gpio_port_index(uint32_t addr) {
    for (int i = 0; i < GPIO_NUM_PORTS; i++) {
        if (addr >= gpio_port_addresses[i] && addr < gpio_port_addresses[i] + 0x400) {
            return i;
        }
    }
    return -1; // Invalid address
}

// Helper function to get register offset within port
static uint32_t get_gpio_register_offset(uint32_t addr) {
    for (int i = 0; i < GPIO_NUM_PORTS; i++) {
        if (addr >= gpio_port_addresses[i] && addr < gpio_port_addresses[i] + 0x400) {
            return addr - gpio_port_addresses[i];
        }
    }
    return 0; // Should not happen
}

uint32_t gpio_read_register(GPIO_State *gpio, uint32_t addr) {
    int port_idx = get_gpio_port_index(addr);
    if (port_idx == -1) {
        //printf("[GPIO] Reading from invalid address 0x%08X\n", addr);
        return 0xFFFFFFFF; // Return error value
    }
    
    uint32_t reg_offset = get_gpio_register_offset(addr);
    GPIO_Port *port = &gpio->ports[port_idx];
    
    switch (reg_offset) {
        case GPIO_CRL_OFFSET:
            return port->crl;
        case GPIO_CRH_OFFSET:
            return port->crh;
        case GPIO_IDR_OFFSET:
            return port->idr;
        case GPIO_ODR_OFFSET:
            return port->odr;
        case GPIO_BSRR_OFFSET:
            // BSRR is read-only (returns current state)
            return port->odr;
        case GPIO_BRR_OFFSET:
            // BRR is read-only (returns current state)
            return port->odr;
        case GPIO_LCKR_OFFSET:
            return port->lckr;
        default:
            //printf("[GPIO] Reading from unknown register offset 0x%02X\n", reg_offset);
            return 0xFFFFFFFF;
    }
}

void gpio_write_register(GPIO_State *gpio, uint32_t addr, uint32_t value) {
    int port_idx = get_gpio_port_index(addr);
    if (port_idx == -1) {
        //printf("[GPIO] Writing to invalid address 0x%08X\n", addr);
        return;
    }
    
    uint32_t reg_offset = get_gpio_register_offset(addr);
    GPIO_Port *port = &gpio->ports[port_idx];
    
    switch (reg_offset) {
        case GPIO_CRL_OFFSET:
            port->crl = value;
            break;
        case GPIO_CRH_OFFSET:
            port->crh = value;
            break;
        case GPIO_ODR_OFFSET:
            port->odr = value;
            // Выводим состояние ODR для оркестратора
            printf("[GPIO] PORT_%c_ODR = 0x%08X\n", 'A' + port_idx, value);
            break;
        case GPIO_BSRR_OFFSET:
            // Handle BSRR write (set/reset bits)
            // Bits 0-15: Reset bits
            // Bits 16-31: Set bits
            if (value & 0xFFFF) {
                // Reset bits
                port->odr &= ~(value & 0xFFFF);
            }
            if (value & 0xFFFF0000) {
                // Set bits
                port->odr |= (value >> 16);
            }
            // Выводим обновлённое состояние ODR
            printf("[GPIO] PORT_%c_ODR = 0x%08X\n", 'A' + port_idx, port->odr);
            break;
        case GPIO_BRR_OFFSET:
            // Handle BRR write (reset bits)
            port->odr &= ~value;
            break;
        case GPIO_LCKR_OFFSET:
            port->lckr = value;
            break;
        default:
            //printf("[GPIO] Writing to unknown register offset 0x%02X\n", reg_offset);
            break;
    }
}