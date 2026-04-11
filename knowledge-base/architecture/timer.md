# TIM6 Timer

> [!summary]
> TIM6 is a basic general-purpose timer with a 16-bit counter, prescaler, and auto-reload register, used for generating periodic interrupts and measuring time intervals.

## Responsibility

The TIM6 timer is responsible for:
- **Counting clock cycles** — incrementing the CNT (counter) register at a rate determined by the PSC (prescaler)
- **Generating update events** — triggering an interrupt when CNT reaches ARR (auto-reload value)
- **One-pulse mode** — stopping after a single cycle (optional)
- **DMA requests** — triggering DMA transfers on update events (not yet implemented)

## TIM6 Register Map

| Register | Offset | Size | R/W | Description |
|----------|--------|------|-----|-------------|
| **CR1** | `0x00` | 16-bit | R/W | Control Register 1 |
| **CR2** | `0x04` | 16-bit | R/W | Control Register 2 |
| **SMCR** | `0x08` | 16-bit | R/W | Slave Mode Control Register |
| **DIER** | `0x0C` | 16-bit | R/W | DMA/Interrupt Enable Register |
| **SR** | `0x10` | 16-bit | R/W | Status Register |
| **EGR** | `0x14` | 16-bit | W | Event Generation Register |
| **CNT** | `0x24` | 16-bit | R/W | Counter |
| **PSC** | `0x28` | 16-bit | R/W | Prescaler |
| **ARR** | `0x2C` | 16-bit | R/W | Auto-Reload Register |

### CR1 — Control Register 1

| Bit | Name | Description |
|-----|------|-------------|
| 0 | **CEN** | Counter Enable (0 = disabled, 1 = enabled) |
| 1 | **UDIS** | Update Disable (0 = UEV enabled, 1 = no update) |
| 2 | **URS** | Update Request Source (0 = any event generates UEV, 1 = only counter overflow) |
| 3 | **OPM** | One-Pulse Mode (0 = continuous, 1 = stop at next update) |
| 4 | **DIR** | Direction (0 = upcounter, 1 = downcounter) |
| 7 | **ARPE** | Auto-Reload Preload Enable (0 = ARR immediate, 1 = ARR buffered) |

### DIER — DMA/Interrupt Enable Register

| Bit | Name | Description |
|-----|------|-------------|
| 0 | **UIE** | Update Interrupt Enable |
| 1 | **TIE** | Trigger Interrupt Enable |
| 8 | **UDE** | Update DMA Enable |

### SR — Status Register

| Bit | Name | Description |
|-----|------|-------------|
| 0 | **UIF** | Update Interrupt Flag (set on overflow) |
| 1 | **TIF** | Trigger Interrupt Flag |

## Timer Operation

### Counting Mechanism

```
Timer Clock (e.g., 72 MHz)
         │
         ▼
   ┌───────────┐
   │   PSC     │  ← Prescaler (divide clock by PSC+1)
   │ (16-bit)  │
   └─────┬─────┘
         │
         ▼ (tick every (PSC+1) timer clocks)
   ┌───────────┐
   │   CNT     │  ← Counter (incremented each tick)
   │ (16-bit)  │
   └─────┬─────┘
         │
         ▼ (when CNT == ARR)
   ┌───────────┐
   │   ARR     │  ← Auto-Reload Value
   │ (16-bit)  │
   └─────┬─────┘
         │
         ▼
   Update Event → Set UIF flag
         │
         ▼ (if UIE=1)
   Interrupt Request → [[interrupt_controller\|NVIC]]
```

### Overflow Calculation

```
Timer tick period = (PSC + 1) / Timer_Clock_Frequency
Overflow period   = (PSC + 1) × (ARR + 1) / Timer_Clock_Frequency

Example: PSC=7199, ARR=9999, Clock=72 MHz
  Tick period = (7199+1) / 72,000,000 = 100 µs
  Overflow    = 100 µs × 10,000 = 1 second
```

## Data Structure (`tim6.h`)

```c
typedef struct {
    uint32_t cr1;    // Control Register 1
    uint32_t cr2;    // Control Register 2
    uint32_t smcr;   // Slave Mode Control Register
    uint32_t dier;   // DMA/Interrupt Enable Register
    uint32_t sr;     // Status Register
    uint32_t egr;    // Event Generation Register
    uint32_t cnt;    // Counter
    uint32_t psc;    // Prescaler
    uint32_t arr;    // Auto Reload Register
    uint32_t base_address;
} TIM6_State;
```

## Register Write Logic

When the CPU writes to the CR1 register:

```c
void tim6_write_register(TIM6_State *tim6, uint32_t addr, uint32_t value) {
    // ... address decoding ...
    tim6->regs.cr1 = value;
    
    // If CEN (Counter Enable) is set, start counting
    if (value & TIM6_CR1_CEN) {
        // Counter increments on each simulator_step() call
        // (driven by the instruction execution cycle)
    }
}
```

## Counter Update

```c
void tim6_update_counter(TIM6_State *tim6) {
    if (!(tim6->regs.cr1 & TIM6_CR1_CEN)) return;  // Not enabled
    
    tim6->regs.cnt++;
    
    if (tim6->regs.cnt >= tim6->regs.arr) {
        // Overflow occurred
        tim6->regs.sr |= TIM6_SR_UIF;  // Set Update Interrupt Flag
        
        // Generate interrupt if enabled
        if (tim6->regs.dier & TIM6_DIER_UIE) {
            // nvic_set_pending(nvic, TIM6_IRQn);  // ← Not yet implemented
        }
        
        // Reset counter (if ARPE=0, immediate reload)
        if (!(tim6->regs.cr1 & TIM6_CR1_ARPE)) {
            tim6->regs.cnt = 0;
        }
        
        // One-pulse mode: stop after this event
        if (tim6->regs.cr1 & TIM6_CR1_OPM) {
            tim6->regs.cr1 &= ~TIM6_CR1_CEN;  // Disable counter
        }
    }
}
```

## Current Implementation Status

| Feature | Status | Notes |
|---------|--------|-------|
| Register storage | ✅ Implemented | All TIM6 registers defined |
| CRL/CRH access | ✅ Implemented | Read/write via `tim6_read_register` / `tim6_write_register` |
| Counter update logic | ⚠️ Stub | `tim6_update_counter` exists but not called from main loop |
| **Interrupt generation** | ❌ Not implemented | NVIC integration missing |
| **Bus routing** | ❌ Not implemented | Memory writes to TIM6 addresses don't reach TIM6 module |
| DMA requests | ❌ Not implemented | UDE bit defined but no DMA logic |

## Interaction with Other Components

| Component | Interaction |
|-----------|-------------|
| [[instruction_execution\|Instruction Executor]] | STR instructions write to TIM6 registers |
| [[interrupt_controller\|NVIC]] | TIM6 sets pending IRQ 22 on overflow (not yet implemented) |
| [[memory_model\|Memory Bus]] | TIM6 registers accessible at `0x40001400` (APB1) |
| [[overall_architecture\|Simulator Main Loop]] | Should call `tim6_update_counter` each cycle (not yet done) |

## Use Cases in Student Labs

1. **Blinking LED** — TIM6 generates periodic interrupts; ISR toggles GPIO pin
2. **Timing measurement** — Student reads CNT to measure elapsed time
3. **Delay generation** — Poll UIF flag or wait for interrupt
4. **PWM basis** — With compare registers (not in TIM6, but in advanced timers)

---

#timer #tim6 #counter #prescaler #auto-reload #interrupts
