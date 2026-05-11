#include <stdint.h>
#include "memory_map.h"

// Forward declaration
typedef struct NVIC_State NVIC_State;

typedef struct {
    uint32_t cr1;       // Control Register 1
    uint32_t cr2;       // Control Register 2
    uint32_t smcr;      // Slave Mode Control Register
    uint32_t dier;      // DMA/Interrupt Enable Register
    uint32_t sr;        // Status Register
    uint32_t egr;       // Event Generation Register
    uint32_t cnt;       // Counter
    uint32_t psc;       // Prescaler
    uint32_t arr;       // Auto Reload Register
} TIM6_Registers;

// TIM6 State Structure
typedef struct {
    TIM6_Registers regs;
    uint32_t base_address;
    uint32_t psc_counter;
    uint64_t update_event_count;
} TIM6_State;