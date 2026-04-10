# GPIO (General Purpose Input/Output) Periphery

## Overview
The GPIO peripheral provides general-purpose input/output functionality for the STM32F103C8T6 microcontroller. It includes multiple ports (A, B, C, D, E, F, G) with configurable pins.

## Memory Map
Each GPIO port has its own register block with the following addresses:
- Port A: 0x40010800 - 0x40010BFF
- Port B: 0x40010C00 - 0x40010FFF  
- Port C: 0x40011000 - 0x400113FF
- Port D: 0x40011400 - 0x400117FF
- Port E: 0x40011800 - 0x40011BFF
- Port F: 0x40011C00 - 0x40011FFF
- Port G: 0x40012000 - 0x400123FF

## Registers
Each GPIO port has the following registers:

### CRL (Configuration Register Low) - Address: 0x00
- Configures pins 0-7
- 4 bits per pin (00=Input, 01=Output 10MHz, 10=Output 2MHz, 11=Output 50MHz)
- 32 bits total (8 pins × 4 bits)

### CRH (Configuration Register High) - Address: 0x04
- Configures pins 8-15
- Same format as CRL
- 32 bits total (8 pins × 4 bits)

### IDR (Input Data Register) - Address: 0x08
- Read-only register
- Contains current state of input pins
- 16 bits total

### ODR (Output Data Register) - Address: 0x0C
- Read/write register
- Contains current state of output pins
- 16 bits total

### BSRR (Bit Set/Reset Register) - Address: 0x10
- Write-only register
- Allows setting/resetting individual output pins
- 32 bits total (16 bits for reset, 16 bits for set)

### BRR (Bit Reset Register) - Address: 0x14
- Write-only register
- Allows resetting individual output pins
- 16 bits total

### LCKR (Lock Key Register) - Address: 0x18
- Configuration lock register
- 32 bits total

## Implementation Plan
1. Create GPIO structure with registers for each port
2. Implement memory mapping for GPIO registers
3. Implement register read/write handlers
4. Add GPIO initialization and reset functions
5. Integrate with memory subsystem