# Interrupt Controller (NVIC)

> [!summary]
> The NVIC (Nested Vectored Interrupt Controller) models the Cortex-M3 interrupt management hardware, supporting up to 80 interrupts with enable/pending/priority tracking.

## Responsibility

The NVIC is responsible for:
- **Interrupt enable/disable** — controlling which interrupts can fire
- **Pending state management** — tracking which interrupts are waiting to be serviced
- **Priority tracking** — storing and comparing interrupt priorities
- **Active state tracking** — knowing which interrupts are currently being serviced

## NVIC Architecture

### Register Model

| Register | Offset | Purpose | Array Size |
|----------|--------|---------|------------|
| **ISER** | `0x000` | Interrupt Set Enable | 3 regs × 32 bits = 96 IRQs |
| **ICER** | `0x080` | Interrupt Clear Enable | 3 regs × 32 bits |
| **ISPR** | `0x100` | Interrupt Set Pending | 3 regs × 32 bits |
| **ICPR** | `0x180` | Interrupt Clear Pending | 3 regs × 32 bits |
| **IABR** | `0x200` | Interrupt Active Bit | 3 regs × 32 bits |
| **IPR** | `0x300` | Interrupt Priority | 80 bytes (one per IRQ) |

### Data Structure (`nvic.h`)

```c
#define NVIC_NUM_INTERRUPTS 80

typedef struct {
    uint32_t iser[NVIC_NUM_INTERRUPTS/32];  // 3 registers (96 bits)
    uint32_t icer[NVIC_NUM_INTERRUPTS/32];
    uint32_t ispr[NVIC_NUM_INTERRUPTS/32];
    uint32_t icpr[NVIC_NUM_INTERRUPTS/32];
    uint32_t iabr[NVIC_NUM_INTERRUPTS/32];
    uint8_t  ipr[NVIC_NUM_INTERRUPTS];      // 80 priority bytes
} NVIC_State;
```

## Interrupt Lifecycle

```
     Peripheral Event
           │
           ▼
    ┌──────────────┐
    │ Set Pending  │  ← nvic_set_pending(irqn)
    │ (ISPR)       │
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │ Check Enable │  ← nvic_is_enabled(irqn) → ISER
    │ (ISER)       │
    └──────┬───────┘
           │ (if enabled)
           ▼
    ┌──────────────┐
    │ Compare      │  ← Find highest priority pending interrupt
    │ Priorities   │     nvic_get_priority(irqn) → IPR
    │ (IPR)        │
    └──────┬───────┘
           │ (highest priority wins)
           ▼
    ┌──────────────┐
    │ CPU Entry    │  ← Save context (stacking), set xPSR.IPSR,
    │              │     branch to vector table entry
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │ ISR Executes │  ← Interrupt Service Routine
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │ EXC_RETURN   │  ← Restore context (unstacking), return to thread
    └──────────────┘
```

## API Functions

| Function | Purpose |
|----------|---------|
| `nvic_init` | Zero-initialize all registers |
| `nvic_enable_interrupt(irqn)` | Set bit in ISER |
| `nvic_disable_interrupt(irqn)` | Set bit in ICER |
| `nvic_set_pending(irqn)` | Set bit in ISPR |
| `nvic_clear_pending(irqn)` | Set bit in ICPR |
| `nvic_is_enabled(irqn)` | Read ISER bit |
| `nvic_is_pending(irqn)` | Read ISPR bit |
| `nvic_is_active(irqn)` | Read IABR bit |
| `nvic_set_priority(irqn, priority)` | Write to IPR |
| `nvic_get_priority(irqn)` | Read from IPR |

## Bit-Level Operations

### Enable Interrupt

```c
void nvic_enable_interrupt(NVIC_State *nvic, uint8_t irqn) {
    uint8_t reg = irqn / 32;
    uint8_t bit = irqn % 32;
    nvic->iser[reg] |= (1U << bit);
}
```

### Check Pending

```c
uint8_t nvic_is_pending(NVIC_State *nvic, uint8_t irqn) {
    uint8_t reg = irqn / 32;
    uint8_t bit = irqn % 32;
    return (nvic->ispr[reg] >> bit) & 1;
}
```

## Integration with CPU Core

The NVIC interacts with the [[cpu_core\|CPU Core]] through:

| Signal | Direction | Description |
|--------|-----------|-------------|
| **PRIMASK** | CPU → NVIC | If PRIMASK=1, interrupts with configurable priority are masked |
| **xPSR.IPSR** | NVIC → CPU | Set to exception number on entry |
| **Vector Table** | NVIC → Memory | Read from `0x08000000` to get ISR address |
| **Stacking/Unstacking** | CPU hardware | Push/pop R0-R3, R12, LR, PC, xPSR automatically |

## Current Implementation Status

| Feature | Status | Notes |
|---------|--------|-------|
| Register storage | ✅ Implemented | All ISER/ICER/ISPR/ICPR/IABR/IPR arrays |
| Enable/disable | ✅ Implemented | Bit-level operations |
| Pending/clear | ✅ Implemented | Bit-level operations |
| Priority storage | ✅ Implemented | 8-bit priority per IRQ |
| **Exception entry** | ❌ Not implemented | No stacking, no vector table lookup |
| **Exception exit** | ❌ Not implemented | No EXC_RETURN handling |
| **Priority comparison** | ❌ Not implemented | No "find highest priority" logic |
| **CPU interrupt signal** | ❌ Not implemented | NVIC doesn't actually interrupt CPU execution |

## Peripheral Interrupt Sources

In the STM32F103, peripherals generate interrupts by setting their IRQ number in the NVIC:

| Peripheral | IRQ Number | Description |
|------------|-----------|-------------|
| **TIM6** | IRQ 22 | Timer update event |
| **USART1** | IRQ 37 | UART transmit/receive |
| **GPIO EXTI** | IRQ 0–15 | External interrupts on GPIO pins |

When a peripheral event occurs (e.g., TIM6 counter overflow), it calls `nvic_set_pending(nvic, TIM6_IRQn)`. The NVIC then (in a full implementation) would signal the CPU to enter the ISR.

---

#nvic #interrupts #cortex-m3 #exception-handling #priority
