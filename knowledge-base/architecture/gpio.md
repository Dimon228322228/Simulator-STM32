# GPIO Ports

> [!summary]
> GPIO (General Purpose Input/Output) ports model the STM32F103's pin configuration and data registers, supporting 7 ports (A–G) with 16 pins each.

## Responsibility

The GPIO subsystem is responsible for:
- **Pin configuration** — setting each pin as input, output, or alternate function via CRL/CRH registers
- **Input data** — reading pin states via IDR (Input Data Register)
- **Output data** — writing pin states via ODR (Output Data Register)
- **Atomic bit manipulation** — setting/resetting individual pins via BSRR (Bit Set/Reset Register) and BRR (Bit Reset Register)
- **Configuration locking** — preventing accidental reconfiguration via LCKR (Lock Key Register)

## GPIO Port Architecture

### Register Map (per port)

| Register | Offset | Size | R/W | Description |
|----------|--------|------|-----|-------------|
| **CRL** | `0x00` | 32-bit | R/W | Configuration Register Low (pins 0–7) |
| **CRH** | `0x04` | 32-bit | R/W | Configuration Register High (pins 8–15) |
| **IDR** | `0x08` | 32-bit | R | Input Data Register (bits 0–15 valid) |
| **ODR** | `0x0C` | 32-bit | R/W | Output Data Register (bits 0–15 valid) |
| **BSRR** | `0x10` | 32-bit | W | Bit Set/Reset Register |
| **BRR** | `0x14` | 32-bit | W | Bit Reset Register |
| **LCKR** | `0x18` | 32-bit | R/W | Lock Key Register |

### Port Base Addresses

| Port | Base Address | Bus |
|------|-------------|-----|
| GPIOA | `0x40010800` | APB2 |
| GPIOB | `0x40010C00` | APB2 |
| GPIOC | `0x40011000` | APB2 |
| GPIOD | `0x40011400` | APB2 |
| GPIOE | `0x40011800` | APB2 |
| GPIOF | `0x40011C00` | APB2 |
| GPIOG | `0x40012000` | APB2 |

## CRL/CRH Configuration

Each pin is configured by a **4-bit field** (2 bits for MODE + 2 bits for CNF):

```
CRL:  [31:28][27:24]...[3:0]  ← Pin 7 ... Pin 0
CRH:  [31:28][27:24]...[3:0]  ← Pin 15 ... Pin 8
```

### MODE Bits (bits [1:0])

| MODE | Configuration |
|------|--------------|
| `00` | Input (reset state) |
| `01` | Output, max speed 10 MHz |
| `10` | Output, max speed 2 MHz |
| `11` | Output, max speed 50 MHz |

### CNF Bits (bits [3:2])

**For input mode (MODE=00):**

| CNF | Configuration |
|-----|--------------|
| `00` | Analog input |
| `01` | Floating input (reset state) |
| `10` | Input with pull-up/pull-down |
| `11` | Reserved |

**For output mode (MODE≠00):**

| CNF | Configuration |
|-----|--------------|
| `00` | General purpose push-pull |
| `01` | General purpose open-drain |
| `10` | Alternate function push-pull |
| `11` | Alternate function open-drain |

## BSRR — Atomic Bit Set/Reset

The BSRR register allows **atomic** (non-read-modify-write) manipulation of individual ODR bits:

```
BSRR: [31:16]  ← Reset bits (write 1 to clear ODR bit)
      [15:0]   ← Set bits   (write 1 to set ODR bit)
```

**Operation:**
- Writing `1` to bit N (0–15) → sets ODR[N] = 1
- Writing `1` to bit N+16 (16–31) → sets ODR[N] = 0
- Writing `0` → no effect
- If both bits N and N+16 are written as `1`, the **set** operation wins

### Implementation (`gpio.c`)

```c
case GPIO_BSRR_OFFSET:
    if (value & 0xFFFF) {
        // Reset bits (bits 0-15 clear ODR bits)
        port->odr &= ~(value & 0xFFFF);
    }
    if (value & 0xFFFF0000) {
        // Set bits (bits 16-31 set ODR bits)
        port->odr |= (value >> 16);
    }
    printf("[GPIO] PORT_%c_ODR = 0x%08X\n", 'A' + port_idx, port->odr);
    break;
```

> [!bug] Implementation Bug in BSRR
> The current implementation has the **set/reset bits swapped**. According to STM32 documentation:
> - Bits 0–15 should **SET** ODR bits
> - Bits 16–31 should **RESET** ODR bits
> 
> The current code does the opposite. This needs to be fixed.

## BRR — Bit Reset Register

BRR is a simpler interface that only resets ODR bits:

```c
case GPIO_BRR_OFFSET:
    port->odr &= ~value;
    break;
```

## Data Structure (`gpio.h`)

```c
typedef struct {
    uint32_t crl;   // Configuration Register Low
    uint32_t crh;   // Configuration Register High
    uint32_t idr;   // Input Data Register
    uint32_t odr;   // Output Data Register
    uint32_t bsrr;  // Bit Set/Reset Register
    uint32_t brr;   // Bit Reset Register
    uint32_t lckr;  // Lock Key Register
} GPIO_Port;

typedef struct {
    GPIO_Port ports[GPIO_NUM_PORTS];           // 7 ports (A–G)
    uint32_t port_addresses[GPIO_NUM_PORTS];   // Base addresses
} GPIO_State;
```

## Logging for Orchestrator

When ODR is written (via ODR, BSRR, or BRR), the GPIO module logs the state:

```c
printf("[GPIO] PORT_%c_ODR = 0x%08X\n", 'A' + port_idx, value);
```

The [[microservice\|orchestrator]] parses this output via regex to extract the final GPIO state for the task result.

## Current Limitations

| Limitation | Impact |
|------------|--------|
| No MODE/CNF enforcement | Writing to ODR works regardless of pin configuration |
| No external input simulation | IDR is never updated from test environment |
| No alternate function routing | GPIO doesn't interact with UART/SPI/I2C peripherals |
| No RCC clock gating | GPIO is always accessible, ignoring RCC_APB2ENR |
| BSRR bit order bug | Set/reset logic is inverted (see note above) |

## Interaction with Other Components

| Component | Interaction |
|-----------|-------------|
| [[instruction_execution\|Instruction Executor]] | STR instructions write to GPIO register addresses |
| [[microservice\|Orchestrator]] | Parses `[GPIO] PORT_X_ODR` log lines for result reporting |
| [[uart\|UART]] | UART TX/RX pins are routed through GPIO alternate function mode |

---

#gpio #peripherals #registers #pin-configuration #bsrr #odr
