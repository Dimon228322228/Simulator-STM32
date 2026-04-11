# Peripherals Implementation

> [!summary]
> Overview of all peripheral modules implemented in the simulator, their register models, and current integration status.

## Peripheral Architecture

All peripherals follow a common design pattern:
1. **State structure** with register shadow storage
2. **Init/reset functions** to set default state
3. **Read/write functions** for register access
4. **Operational functions** (transmit, count, etc.) — partially implemented

## Implemented Peripherals

### [[gpio\|GPIO]] — General Purpose I/O

**Status:** ✅ Registers defined, ✅ read/write implemented, ❌ bus routing missing

| Feature | Status |
|---------|--------|
| CRL/CRH configuration | ✅ Defined |
| IDR read | ✅ Returns stored value |
| ODR write | ✅ Updates state, logs to stdout |
| BSRR atomic set/reset | ✅ Implemented (with bit order bug) |
| BRR reset | ✅ Implemented |
| LCKR lock | ✅ Defined |
| Bus routing | ❌ Memory writes to GPIO addresses dropped |

**Ports:** A, B, C, D, E, F, G (7 ports, 16 pins each)

### [[timer\|TIM6]] — Basic Timer

**Status:** ✅ Registers defined, ❌ counter update not integrated

| Feature | Status |
|---------|--------|
| CR1 control | ✅ Defined |
| CNT counter | ✅ Defined |
| PSC prescaler | ✅ Defined |
| ARR auto-reload | ✅ Defined |
| DIER interrupt enable | ✅ Defined |
| SR status | ✅ Defined |
| Counter update logic | ⚠️ Function exists but not called |
| Interrupt generation | ❌ NVIC integration missing |

### [[uart\|USART]] — Serial Communication

**Status:** ✅ Registers defined, ❌ transmission not triggered

| Feature | Status |
|---------|--------|
| SR status register | ✅ Defined |
| DR data register | ✅ Defined |
| BRR baud rate | ✅ Defined |
| CR1/CR2/CR3 control | ✅ Defined |
| TX/RX buffers (256 bytes each) | ✅ Defined |
| Circular buffer management | ✅ Head/tail pointers defined |
| Transmission trigger | ❌ DR write doesn't output |
| Reception simulation | ❌ No external input mechanism |

**Instances:** USART1 (APB2), USART2 (APB1), USART3 (APB1)

### [[interrupt_controller\|NVIC]] — Interrupt Controller

**Status:** ✅ Registers defined, ❌ exception entry/exit not implemented

| Feature | Status |
|---------|--------|
| ISER/ICER enable | ✅ Bit-level operations |
| ISPR/ICPR pending | ✅ Bit-level operations |
| IABR active | ✅ Defined |
| IPR priority | ✅ 8-bit per IRQ |
| Enable/disable/pending API | ✅ Functions implemented |
| Exception entry | ❌ No stacking, no vector lookup |
| Exception exit | ❌ No EXC_RETURN handling |
| Priority comparison | ❌ No "find highest" logic |

### SPI — Serial Peripheral Interface

**Status:** ✅ Registers defined, ❌ no data transfer logic

| Feature | Status |
|---------|--------|
| CR1/CR2 control | ✅ Defined |
| SR status | ✅ Defined |
| DR data | ✅ Defined |
| TX/RX buffers | ✅ Defined |
| transmit/receive functions | ✅ Stubs defined |
| Master/slave mode | ❌ Not implemented |

**Instances:** SPI1 (APB2), SPI2 (APB1), SPI3 (APB1)

### I2C — Inter-Integrated Circuit

**Status:** ✅ Registers defined, ❌ no bus communication

| Feature | Status |
|---------|--------|
| CR1/CR2 control | ✅ Defined |
| SR1/SR2 status | ✅ Defined |
| OAR1/OAR2 own address | ✅ Defined |
| CCR clock control | ✅ Defined |
| TX/RX buffers | ✅ Defined |
| transmit/receive functions | ✅ Stubs defined |
| Master/slave mode switching | ❌ Not implemented |
| Address matching | ❌ Not implemented |

**Instances:** I2C1, I2C2 (both APB1)

### DMA — Direct Memory Access

**Status:** ✅ Registers defined, ❌ no transfer logic

| Feature | Status |
|---------|--------|
| ISR status | ✅ Defined |
| IFCR flag clear | ✅ Defined |
| CCR channel control | ✅ 7 channels defined |
| CNDTR data count | ✅ Defined |
| CPAR/CMAR addresses | ✅ Defined |
| start_transfer function | ✅ Stub |
| Transfer execution | ❌ No actual memory copy |

**Instance:** DMA1 (7 channels)

### RCC — Reset and Clock Control

**Status:** ✅ Registers defined, ❌ no clock gating enforcement

| Feature | Status |
|---------|--------|
| CR clock control | ✅ Defined |
| CFGR configuration | ✅ Defined |
| APB2ENR/APB1ENR enable | ✅ Defined |
| enable/disable peripheral | ✅ Functions defined |
| Clock gating enforcement | ❌ Peripherals always accessible |

### Bus Matrix

**Status:** ✅ Shadow registers defined, ❌ no actual routing

| Feature | Status |
|---------|--------|
| AHB/APB address tracking | ✅ Base addresses stored |
| RCC clock shadow | ✅ Control registers stored |
| DMA state shadow | ✅ ISR/IFCR/channels stored |
| Read/write functions | ✅ Stubs return 0 |
| Accessibility check | ✅ Stub returns 0 |

## Peripheral Initialization Sequence

In `main.c`, all peripherals are initialized during startup:

```c
// Initialize peripherals
gpio_init(&sim.gpio);
tim6_init(&sim.tim6);
nvic_init(&sim.nvic);
bus_matrix_init(&sim.bus);
rcc_init(&sim.rcc);
dma_init(&sim.dma);

// Initialize communication interfaces
usart_init(&sim.usart1, USART1_BASE_ADDR);
usart_init(&sim.usart2, USART2_BASE_ADDR);
usart_init(&sim.usart3, USART3_BASE_ADDR);
spi_init(&sim.spi1, SPI1_BASE_ADDR);
spi_init(&sim.spi2, SPI2_BASE_ADDR);
spi_init(&sim.spi3, SPI3_BASE_ADDR);
i2c_init(&sim.i2c1, I2C1_BASE_ADDR);
i2c_init(&sim.i2c2, I2C2_BASE_ADDR);
```

Each `*_init()` function:
1. Zeros all registers via `memset` or explicit assignment
2. Sets base address fields
3. Returns void (no failure mode)

## Missing Bus Routing — Critical Gap

The **most significant gap** is that memory writes to peripheral addresses (`0x40000000+`) are not routed to the appropriate peripheral modules. 

**Current behavior:**
```c
void memory_write_byte(Memory *mem, uint32_t addr, uint8_t value) {
    if (addr >= SRAM_BASE_ADDR && addr < SRAM_BASE_ADDR + SRAM_SIZE) {
        mem->sram[addr - SRAM_BASE_ADDR] = value;
        return;
    }
    // Flash writes ignored
    // Peripheral writes → silently dropped
}
```

**Required behavior:**
```c
void memory_write_byte(Memory *mem, uint32_t addr, uint8_t value, Simulator *sim) {
    if (addr >= SRAM_BASE && addr < SRAM_BASE + SRAM_SIZE) {
        mem->sram[addr - SRAM_BASE] = value;
    }
    else if (addr >= GPIOA_BASE && addr < GPIOA_BASE + 0x400) {
        // Route to GPIO — but GPIO works on 32-bit registers, not bytes
        uint32_t offset = addr % 4;
        uint32_t reg_addr = addr & ~0x3;
        uint32_t current = gpio_read_register(&sim->gpio, reg_addr);
        current = (current & ~(0xFF << (offset * 8))) | (value << (offset * 8));
        gpio_write_register(&sim->gpio, reg_addr, current);
    }
    else if (addr >= USART1_BASE && ...) {
        usart_write_register(&sim->usart1, addr, value);
    }
    // ... etc for all peripherals
}
```

## Peripheral Address Map

```
APB1 (0x40000000 – 0x4000FFFF):
  0x40001400  TIM6
  0x40003800  SPI2
  0x40003C00  SPI3
  0x40004400  USART2
  0x40004800  USART3
  0x40005400  I2C1
  0x40005800  I2C2
  0x40020000  DMA1

APB2 (0x40010000 – 0x4001FFFF):
  0x40010800  GPIOA
  0x40010C00  GPIOB
  0x40011000  GPIOC
  0x40011400  GPIOD
  0x40011800  GPIOE
  0x40011C00  GPIOF
  0x40012000  GPIOG
  0x40013000  SPI1
  0x40021000  RCC
```

## Integration Priority

| Priority | Peripheral | Reason |
|----------|-----------|--------|
| 🔴 Critical | GPIO | Required for LED blink labs |
| 🔴 Critical | USART1 | Required for UART output labs |
| 🟡 High | TIM6 | Required for timer interrupt labs |
| 🟡 High | NVIC | Required for all interrupt-based labs |
| 🟢 Medium | RCC | Clock gating affects power management |
| 🟢 Low | SPI/I2C | Advanced communication labs (future) |
| 🟢 Low | DMA | Advanced memory transfer labs (future) |

---

#peripherals #gpio #timer #uart #nvic #spi #i2c #dma #rcc #bus-matrix #integration
