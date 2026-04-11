# Glossary

> [!summary]
> Terms, abbreviations, and definitions used throughout the STM32 simulator project.

## A

| Term | Definition |
|------|-----------|
| **AHB** | Advanced High-performance Bus — ARM AMBA bus for high-throughput peripherals |
| **APB** | Advanced Peripheral Bus — ARM AMBA bus for low-throughput peripherals |
| **ALU** | Arithmetic Logic Unit — CPU component performing math and logic operations |
| **ARR** | Auto-Reload Register — timer register determining overflow point |
| **ARM** | Advanced RISC Machines — CPU architecture designer |

## B

| Term | Definition |
|------|-----------|
| **BKPT** | Breakpoint instruction — causes debugger trap |
| **BL** | Branch with Link — subroutine call instruction (32-bit Thumb-2) |
| **BLX** | Branch with Link and Exchange — call subroutine and switch ARM/Thumb state |
| **BRR** | Bit Reset Register (GPIO) — resets individual ODR bits; Baud Rate Register (USART) |
| **BSRR** | Bit Set/Reset Register (GPIO) — atomic set/clear of ODR bits |
| **BX** | Branch indirect — jump to address in register |

## C

| Term | Definition |
|------|-----------|
| **CEN** | Counter Enable — TIM6 CR1 bit enabling timer operation |
| **CMP** | Compare instruction — subtracts without storing result (flags only) |
| **CNT** | Counter register — TIM6 current count value |
| **Cortex-M3** | ARM processor core implementing ARMv7-M architecture |
| **CRL** | Configuration Register Low — GPIO pins 0-7 configuration |
| **CRH** | Configuration Register High — GPIO pins 8-15 configuration |

## D

| Term | Definition |
|------|-----------|
| **DMA** | Direct Memory Access — hardware memory transfer without CPU intervention |
| **DR** | Data Register — USART transmit/receive data |
| **DIER** | DMA/Interrupt Enable Register — timer interrupt/DMA control |

## E

| Term | Definition |
|------|-----------|
| **EXC_RETURN** | Special LR value used to return from exception (0xFFFFFFF1/9/D) |
| **EPSR** | Execution Program Status Register — part of xPSR |

## F

| Term | Definition |
|------|-----------|
| **Flash** | Non-volatile memory storing firmware (64 KB in STM32F103C8T6) |
| **FP** | Frame Pointer — R11, points to stack frame base |

## G

| Term | Definition |
|------|-----------|
| **GPIO** | General Purpose Input/Output — programmable I/O pins |

## I

| Term | Definition |
|------|-----------|
| **I2C** | Inter-Integrated Circuit — 2-wire synchronous serial protocol |
| **IDR** | Input Data Register (GPIO) — current pin states; Interrupt Disable Register (NVIC) |
| **IP** | Intra-Procedure call scratch register — R12 |
| **IPSR** | Interrupt Program Status Register — part of xPSR, holds current exception number |
| **IPR** | Interrupt Priority Registers (NVIC) — 8-bit priority per IRQ |

## K

| Term | Definition |
|------|-----------|
| **KeyDB** | Multi-threaded Redis fork used as task queue |

## L

| Term | Definition |
|------|-----------|
| **LDR** | Load Register — memory-to-register transfer |
| **LR** | Link Register — R14, holds return address after subroutine call |
| **LSB** | Least Significant Byte — byte at lowest address in little-endian |

## M

| Term | Definition |
|------|-----------|
| **MSP** | Main Stack Pointer — stack used in handler mode and after reset |
| **MOVS** | Move immediate to register (Thumb-16) |

## N

| Term | Definition |
|------|-----------|
| **NVIC** | Nested Vectored Interrupt Controller — ARM interrupt management hardware |
| **NMI** | Non-Maskable Interrupt — highest priority exception (cannot be disabled) |

## O

| Term | Definition |
|------|-----------|
| **ODR** | Output Data Register (GPIO) — current output pin states |

## P

| Term | Definition |
|------|-----------|
| **PC** | Program Counter — R15, address of next instruction |
| **PSC** | Prescaler — timer register dividing clock frequency |
| **PSP** | Process Stack Pointer — stack used in thread mode (user applications) |
| **PWM** | Pulse Width Modulation — variable duty-cycle signal (not yet implemented) |

## R

| Term | Definition |
|------|-----------|
| **RCC** | Reset and Clock Control — manages peripheral clocks and system clock |
| **RESP** | Redis Serialization Protocol — text-based protocol for Redis commands |
| **RXNE** | Read Data Register Not Empty — USART flag indicating received data available |

## S

| Term | Definition |
|------|-----------|
| **SP** | Stack Pointer — R13, current stack top |
| **SPI** | Serial Peripheral Interface — 4-wire synchronous serial protocol |
| **SR** | Status Register — peripheral status flags |
| **SRAM** | Static Random Access Memory — volatile read/write memory (20 KB) |
| **STR** | Store Register — register-to-memory transfer |
| **SysTick** | System Timer — 24-bit countdown timer in Cortex-M core |

## T

| Term | Definition |
|------|-----------|
| **TC** | Transmission Complete — USART flag |
| **TIM6** | Timer 6 — basic 16-bit timer in STM32F103 |
| **Thumb** | ARM compressed instruction set (16-bit) |
| **Thumb-2** | Mixed 16/32-bit ARM instruction set (Cortex-M3) |
| **TXE** | Transmit Data Register Empty — USART flag |

## U

| Term | Definition |
|------|-----------|
| **UE** | USART Enable — CR1 bit enabling peripheral |
| **UIE** | Update Interrupt Enable — TIM6 DIER bit |
| **UIF** | Update Interrupt Flag — TIM6 SR bit set on overflow |
| **USART** | Universal Synchronous/Asynchronous Receiver/Transmitter |

## X

| Term | Definition |
|------|-----------|
| **xPSR** | Program Status Register — combined APSR + IPSR + EPSR |

---

## Flag Definitions

| Flag | Register | Bit | Meaning |
|------|----------|-----|---------|
| **N** | xPSR | 31 | Negative — result has MSB set |
| **Z** | xPSR | 30 | Zero — result is zero |
| **C** | xPSR | 29 | Carry — unsigned overflow |
| **V** | xPSR | 28 | Overflow — signed overflow |

---

## Address Reference

| Symbol | Address | Description |
|--------|---------|-------------|
| `FLASH_BASE_ADDR` | `0x08000000` | Flash memory start |
| `SRAM_BASE_ADDR` | `0x20000000` | SRAM start |
| `PERIPH_BASE_ADDR` | `0x40000000` | Peripheral region start |
| `GPIOA_BASE` | `0x40010800` | GPIO Port A |
| `TIM6_BASE` | `0x40001400` | Timer 6 |
| `USART1_BASE` | `0x40011000` | USART 1 |
| `NVIC_BASE` | `0xE000E100` | NVIC |
| `RCC_BASE` | `0x40021000` | Reset and Clock Control |

---

#glossary #terms #abbreviations #definitions #reference
