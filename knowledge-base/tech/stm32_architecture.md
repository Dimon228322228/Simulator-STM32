# STM32 Architecture

> [!summary]
> The STM32F103C8T6 (Blue Pill) is an ARM Cortex-M3 based microcontroller with 64 KB Flash, 20 KB SRAM, and rich peripheral set, serving as the target platform for the simulator.

## STM32F103C8T6 Overview

| Parameter | Value |
|-----------|-------|
| **Core** | ARM Cortex-M3 |
| **Max Frequency** | 72 MHz |
| **Flash** | 64 KB (0x08000000 – 0x0800FFFF) |
| **SRAM** | 20 KB (0x20000000 – 0x20004FFF) |
| **GPIO Ports** | A, B, C (plus D, E on larger packages) |
| **Timers** | TIM1–TIM4 (advanced), TIM5–TIM7 (basic) |
| **USART** | USART1–USART3 |
| **SPI** | SPI1, SPI2 |
| **I2C** | I2C1, I2C2 |
| **ADC** | 2 × 12-bit ADC |
| **DMA** | 7-channel DMA1 |
| **NVIC** | 80 IRQ lines, 16 priority levels |

## Memory Map

```
┌──────────────────────────────────────────┐
│ 0xFFFFFFFF                                │
│ ...                                       │
│ 0xE0000000  System peripherals            │
│             NVIC, SysTick, SCB            │
│ ...                                       │
│ 0x40020000  AHB bus peripherals           │
│             RCC, DMA, Flash interface     │
│ 0x40010000  APB2 bus peripherals          │
│             GPIOA-G, USART1, SPI1, TIM1   │
│ 0x40000000  APB1 bus peripherals          │
│             TIM2-7, USART2-3, SPI2-3,     │
│             I2C1-2, BKP, PWR              │
│ ...                                       │
│ 0x20004FFF  SRAM end                      │
│ 0x20000000  SRAM (20 KB)                  │
│ ...                                       │
│ 0x0800FFFF  Flash end                     │
│ 0x08000000  Flash (64 KB)                 │
│ ...                                       │
│ 0x00000000  Boot memory (aliased)         │
└──────────────────────────────────────────┘
```

## Bus Architecture

```
┌─────────────────────────────────────────┐
│          Cortex-M3 Core                  │
│         (ICode, DCode, System)           │
└─────────────────┬───────────────────────┘
                  │
           ┌──────┴──────┐
           │  AHB Matrix  │
           └──────┬──────┘
                  │
      ┌───────────┼───────────┐
      │           │           │
   ┌──┴──┐   ┌───┴───┐  ┌───┴───┐
   │Flash│   │  DMA  │  │  RCC  │
   └─────┘   └───────┘  └───────┘
                  │
           ┌──────┴──────┐
           │   AHB→APB    │
           │  Bridges     │
           └──────┬──────┘
                  │
      ┌───────────┼───────────┐
      │                       │
  ┌───┴───┐              ┌───┴───┐
  │ APB1  │              │ APB2  │
  │36 MHz │              │72 MHz │
  └───┬───┘              └───┬───┘
      │                      │
  TIM2-7                 GPIOA-G
  USART2-3               USART1
  SPI2-3                 SPI1
  I2C1-2                 TIM1
  BKP, PWR               ADC1-2
```

## Clock System

### Clock Sources

| Source | Frequency | Usage |
|--------|-----------|-------|
| **HSI** | 8 MHz (internal RC) | Default after reset |
| **HSE** | 8 MHz (external crystal) | High-precision timing |
| **PLL** | 72 MHz (derived) | System clock |

### Clock Tree

```
HSI (8 MHz) ──┐
              ├──► PLL ──► 72 MHz (SYSCLK)
HSE (8 MHz) ──┘
                 │
            ┌────┴────┐
            │  AHB     │
            │ Prescaler│
            └────┬────┘
                 │ 72 MHz
            ┌────┴──────────┐
            │               │
       ┌────┴────┐    ┌────┴────┐
       │  APB1   │    │  APB2   │
       │ Prescaler│    │ Prescaler│
       └────┬────┘    └────┬────┘
            │ 36 MHz       │ 72 MHz
      (TIM2-7,       (GPIO, USART1,
       USART2-3,    SPI1, TIM1)
       SPI2-3,
       I2C1-2)
```

## Reset Sequence

1. **Power-on reset** → All clocks enabled, peripherals disabled
2. **Read vector table** at `0x08000000`:
   - Word 0: Initial MSP value
   - Word 1: Reset handler address (initial PC)
3. **Set MSP** from vector table
4. **Set PC** to reset handler
5. **Execute** reset handler (student code)

## Vector Table

```
Address      Content
0x08000000   Initial MSP (e.g., 0x20005000)
0x08000004   Reset_Handler address
0x08000008   NMI_Handler address
0x0800000C   HardFault_Handler address
...
0x08000040   TIM6_IRQHandler address  (IRQ 22)
0x08000094   USART1_IRQHandler address (IRQ 37)
...
```

## GPIO Configuration

Each GPIO pin is configured by 4 bits (MODE[1:0] + CNF[3:2]):

### MODE Bits

| MODE | Configuration |
|------|--------------|
| 00 | Input (reset state) |
| 01 | Output, 10 MHz |
| 10 | Output, 2 MHz |
| 11 | Output, 50 MHz |

### CNF Bits (for output mode)

| CNF | Configuration |
|-----|--------------|
| 00 | General purpose push-pull |
| 01 | General purpose open-drain |
| 10 | Alternate function push-pull |
| 11 | Alternate function open-drain |

## Peripheral Register Access Pattern

All peripherals are accessed via **memory-mapped I/O** at specific addresses:

```c
// Example: Set GPIOA pin 0 to output push-pull at 50 MHz
*(volatile uint32_t *)0x40010800 = 0x00000001;  // CRL: PIN0 = 0001

// Example: Set GPIOA pin 0 high
*(volatile uint32_t *)0x4001080C = 0x00000001;  // ODR: PIN0 = 1

// Example: Configure USART1 baud rate (115200 @ 72 MHz)
*(volatile uint32_t *)0x40011008 = 0x0271;  // BRR: 72M/115200 = 625
```

In the simulator, these accesses are intercepted and handled by peripheral modules rather than actual hardware.

## STM32 vs Simulator Mapping

| STM32 Hardware | Simulator Implementation |
|----------------|-------------------------|
| Flash memory | `calloc(64 KB)` in `memory.c` |
| SRAM | `calloc(20 KB)` in `memory.c` |
| CPU registers | `CPU_State.regs[16]` |
| GPIO ports | `GPIO_State.ports[7]` |
| NVIC | `NVIC_State` (ISER, ICER, ISPR, IPR arrays) |
| TIM6 | `TIM6_State` (CR1, CNT, PSC, ARR) |
| USART1-3 | `USART_State` (SR, DR, BRR, CR1-3) |
| Bus matrix | `Bus_Matrix_State` (shadow registers) |
| System clock | Instruction step counter (not real time) |

---

#stm32 #blue-pill #cortex-m3 #memory-map #clock #gpio #peripherals #arm
