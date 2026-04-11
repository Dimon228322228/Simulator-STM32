# UART/USART

> [!summary]
> The UART (Universal Asynchronous Receiver/Transmitter) subsystem emulates three USART peripherals (USART1, USART2, USART3), providing serial communication with configurable baud rate, parity, and data format.

## Responsibility

The UART subsystem is responsible for:
- **Serial transmission** — CPU writes data to DR, simulator outputs byte to console/log
- **Serial reception** — External input provides bytes, simulator sets RXNE flag
- **Baud rate configuration** — BRR register controls transmission timing
- **Interrupt generation** — TXE (transmit empty) and RXNE (receive not empty) interrupts
- **Status flag management** — SR register tracks transmission/reception state

## USART Register Map

| Register | Offset | Size | R/W | Description |
|----------|--------|------|-----|-------------|
| **SR** | `0x00` | 16-bit | R/W | Status Register |
| **DR** | `0x04` | 16-bit | R/W | Data Register |
| **BRR** | `0x08` | 16-bit | R/W | Baud Rate Register |
| **CR1** | `0x0C` | 16-bit | R/W | Control Register 1 |
| **CR2** | `0x10` | 16-bit | R/W | Control Register 2 |
| **CR3** | `0x14` | 16-bit | R/W | Control Register 3 |
| **GTPR** | `0x18` | 16-bit | R/W | Guard Time and Prescaler |

### Base Addresses

| Peripheral | Base Address | Bus |
|------------|-------------|-----|
| **USART1** | `0x40011000` | APB2 |
| **USART2** | `0x40004400` | APB1 |
| **USART3** | `0x40004800` | APB1 |

### SR — Status Register

| Bit | Name | Description |
|-----|------|-------------|
| 5 | **RXNE** | Read Data Register Not Empty (1 = data available) |
| 6 | **TC** | Transmission Complete (1 = transmission finished) |
| 7 | **TXE** | Transmit Data Register Empty (1 = ready for next byte) |

### CR1 — Control Register 1

| Bit | Name | Description |
|-----|------|-------------|
| 0 | **SBK** | Send Break (1 = send break characters) |
| 2 | **RE** | Receiver Enable (1 = enable receiver) |
| 3 | **TE** | Transmitter Enable (1 = enable transmitter) |
| 5 | **RXNEIE** | RXNE Interrupt Enable |
| 6 | **TCIE** | Transmission Complete Interrupt Enable |
| 7 | **TXEIE** | TXE Interrupt Enable |
| 13 | **UE** | USART Enable (1 = enable peripheral) |

## Data Structure (`usart.h`)

```c
typedef struct {
    uint32_t sr;       // Status register
    uint32_t dr;       // Data register
    uint32_t brr;      // Baud rate register
    uint32_t cr1;      // Control register 1
    uint32_t cr2;      // Control register 2
    uint32_t cr3;      // Control register 3
    uint32_t gtpr;     // Guard time and prescaler
    
    uint8_t tx_buffer[256];  // Transmit buffer (circular)
    uint8_t rx_buffer[256];  // Receive buffer (circular)
    uint32_t tx_head;        // Transmit buffer head pointer
    uint32_t tx_tail;        // Transmit buffer tail pointer
    uint32_t rx_head;        // Receive buffer head pointer
    uint32_t rx_tail;        // Receive buffer tail pointer
    uint32_t tx_count;       // Bytes in transmit buffer
    uint32_t rx_count;       // Bytes in receive buffer
    
    uint32_t base_address;
} USART_State;
```

## Transmission Flow

```
CPU writes to DR register
         │
         ▼
   ┌──────────────┐
   │ Check TE=1?  │  ← Transmitter must be enabled
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │ Set TXE=0    │  ← Transmit register now busy
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │ Output byte  │  → printf("[UART] TX: 0x%02X\n", data)
   │ to console   │
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │ Set TXE=1    │  ← Ready for next byte
   │ Set TC=1     │  ← Transmission complete
   └──────────────┘
          │
          ▼ (if TXEIE=1)
   ┌──────────────┐
   │ Set pending  │  → [[interrupt_controller\|NVIC]]
   │ in NVIC      │
   └──────────────┘
```

## Reception Flow

```
External input (test framework / orchestrator)
         │
         ▼
   ┌──────────────┐
   │ Check RE=1?  │  ← Receiver must be enabled
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │ Write byte   │
   │ to rx_buffer │
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │ Set RXNE=1   │  ← Data available
   └──────┬───────┘
          │
          ▼ (if RXNEIE=1)
   ┌──────────────┐
   │ Set pending  │  → NVIC
   │ in NVIC      │
   └──────────────┘
          │
          ▼
   CPU reads DR
          │
          ▼
   ┌──────────────┐
   │ Clear RXNE=0 │  ← Data consumed
   └──────────────┘
```

## Implementation (`usart.c` — conceptual)

```c
void usart_transmit(USART_State *usart, uint8_t data) {
    if (!(usart->regs.cr1 & USART_CR1_TE)) return;  // Transmitter disabled
    
    usart->regs.dr = data;
    usart->regs.sr &= ~USART_SR_TXE;  // Clear TXE (busy)
    
    // Output to console for orchestrator parsing
    printf("[UART] TX: 0x%02X\n", data);
    
    usart->regs.sr |= USART_SR_TXE | USART_SR_TC;  // Set TXE and TC
    
    // Generate interrupt if enabled
    if (usart->regs.cr1 & USART_CR1_TXEIE) {
        // nvic_set_pending(nvic, USART_IRQn);
    }
}

uint8_t usart_receive(USART_State *usart) {
    if (!(usart->regs.cr1 & USART_CR1_RE)) return 0;  // Receiver disabled
    
    uint8_t data = usart->regs.dr & 0xFF;
    usart->regs.sr &= ~USART_SR_RXNE;  // Clear RXNE
    
    return data;
}
```

## Baud Rate Calculation

```
BRR = f_clk / baud_rate

Example: f_clk = 72 MHz, desired baud = 115200
  BRR = 72,000,000 / 115,200 = 625 = 0x271

BRR format (for USART1 on APB2):
  Bits [15:4] = Mantissa (0x27 = 39)
  Bits [3:0]  = Fraction (0x1 = 1)
  Actual value = 39 + 1/16 = 39.0625
  Actual baud = 72,000,000 / 39.0625 ≈ 115,200
```

## Current Implementation Status

| Feature | Status | Notes |
|---------|--------|-------|
| Register storage | ✅ Implemented | All USART registers defined in `usart.h` |
| Initialization | ✅ Implemented | `usart_init` zeros all registers |
| **Write routing** | ❌ Not implemented | Memory writes to USART addresses don't reach USART module |
| **DR write handling** | ❌ Not implemented | CPU writes to DR don't trigger transmission |
| **RXNE simulation** | ❌ Not implemented | No external input mechanism |
| **Interrupt generation** | ❌ Not implemented | NVIC integration missing |
| **Console output** | ⚠️ Planned | Should output via `printf("[UART] TX: ...")` |

## Interaction with Other Components

| Component | Interaction |
|-----------|-------------|
| [[instruction_execution\|Instruction Executor]] | STR instructions write to USART DR register |
| [[interrupt_controller\|NVIC]] | USART sets pending IRQ 37 (USART1), 38 (USART2), 39 (USART3) |
| [[microservice\|Orchestrator]] | Parses `[UART] TX: 0xXX` log lines for result reporting |
| [[gpio\|GPIO]] | USART TX/RX pins routed through GPIO alternate function |

## Use Cases in Student Labs

1. **Hello World** — Student writes to DR, simulator outputs text to console
2. **Echo server** — Receive byte via interrupt, transmit it back
3. **Formatted output** — Implement `printf`-like function over UART
4. **Binary protocol** — Send/receive structured data packets

---

#uart #usart #serial #communication #baud-rate #interrupts
