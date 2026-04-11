# Unit Tests

> [!summary]
> Unit tests verify the correctness of individual functions and modules in isolation, focusing on the C simulator core.

## Test Files

### `test/test_simulator.c`

Tests the core simulator functionality: memory, CPU state, and instruction execution.

#### Test 1: Basic Simulator Initialization

```c
int test_simulator_basic() {
    Simulator sim;
    
    // Initialize memory
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    // Initialize GPIO
    gpio_init(&sim.gpio);
    
    // Initialize CPU
    cpu_reset(&sim.cpu);
    
    // Verify CPU registers are zeroed
    for (int i = 0; i < 16; i++) {
        if (sim.cpu.regs[i] != 0) {
            printf("FAIL: CPU register R%d not reset correctly\n", i);
            memory_free(&sim.mem);
            return 1;
        }
    }
    
    // Verify special registers
    if (sim.cpu.xpsr != 0 || sim.cpu.primask != 0 ||
        sim.cpu.faultmask != 0 || sim.cpu.basepri != 0 ||
        sim.cpu.control != 0 || sim.cpu.msp != 0 || sim.cpu.psp != 0) {
        printf("FAIL: Special CPU registers not reset correctly\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: Basic simulator test\n");
    memory_free(&sim.mem);
    return 0;
}
```

**What's tested:**
- `memory_init()` — Flash and SRAM allocation
- `gpio_init()` — GPIO structure initialization
- `cpu_reset()` — All registers zeroed

**Assertions:**
- R0–R15 = 0
- xPSR = 0
- PRIMASK = FAULTMASK = BASEPRI = CONTROL = 0
- MSP = PSP = 0
- PC = 0

#### Test 2: Memory Access

```c
int test_memory_access() {
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    // Test Flash read (via memcpy)
    uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78};
    memcpy(sim.mem.flash, test_data, sizeof(test_data));
    
    uint8_t read_data[4];
    memcpy(read_data, sim.mem.flash, sizeof(read_data));
    
    if (memcmp(test_data, read_data, sizeof(test_data)) != 0) {
        printf("FAIL: Flash memory read/write test failed\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    // Test SRAM read/write
    uint8_t sram_test_data[] = {0xAB, 0xCD, 0xEF, 0x01};
    memcpy(sim.mem.sram, sram_test_data, sizeof(sram_test_data));
    
    uint8_t sram_read_data[4];
    memcpy(sram_read_data, sim.mem.sram, sizeof(sram_read_data));
    
    if (memcmp(sram_test_data, sram_read_data, sizeof(sram_test_data)) != 0) {
        printf("FAIL: SRAM memory read/write test failed\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: Memory access test\n");
    memory_free(&sim.mem);
    return 0;
}
```

**What's tested:**
- Flash allocation and read access
- SRAM allocation and read/write access
- Data integrity (no corruption)

**Limitations:**
- Uses `memcpy` instead of `memory_read_byte` / `memory_write_byte`
- Doesn't test address decoding
- Doesn't test little-endian byte ordering

#### Test 3: Instruction Execution

```c
int test_instruction_execution() {
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Failed to initialize memory\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);
    
    // Load: MOV R0, #5
    uint16_t program[] = {0x2005};
    memcpy(sim.mem.flash, program, sizeof(program));
    
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    // Execute one instruction
    simulator_step(&sim);
    
    // Verify result
    if (sim.cpu.regs[0] != 5) {
        printf("FAIL: Expected R0=5, got R0=%u\n", sim.cpu.regs[0]);
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: Instruction execution test\n");
    memory_free(&sim.mem);
    return 0;
}
```

**What's tested:**
- `simulator_step()` — fetch-decode-execute cycle
- `MOV Rd, #imm8` instruction (opcode `0b00100`)
- Register write-back
- PC advancement (PC += 2)

### `test/test_gpio.c`

Tests GPIO register read/write operations.

#### Test Cases

| Test | Operation | Expected Result |
|------|-----------|-----------------|
| Init | `gpio_init()` | All registers = 0 |
| CRL write | `gpio_write_register(GPIOA_CRL, 0x12345678)` | `ports[0].crl = 0x12345678` |
| CRH write | `gpio_write_register(GPIOA_CRH, 0xDEADBEEF)` | `ports[0].crh = 0xDEADBEEF` |
| ODR write | `gpio_write_register(GPIOA_ODR, 0xFFFF)` | `ports[0].odr = 0xFFFF`, log printed |
| ODR read | `gpio_read_register(GPIOA_ODR)` | Returns `0xFFFF` |
| BSRR set | `gpio_write_register(GPIOA_BSRR, 0x00010000)` | `odr |= 0x0001` |
| BSRR reset | `gpio_write_register(GPIOA_BSRR, 0x00020000)` | `odr &= ~0x0002` |
| BRR reset | `gpio_write_register(GPIOA_BRR, 0x0004)` | `odr &= ~0x0004` |
| Invalid address | `gpio_read_register(0x99999999)` | Returns `0xFFFFFFFF` |

## Running Unit Tests

### Linux/macOS

```bash
cd build
./test_simulator
./test_gpio
```

### Windows

```powershell
cd build
.\test_simulator.exe
.\test_gpio.exe
```

### Expected Output

```
Running simulator tests...

Testing basic simulator functionality...
PASS: Basic simulator test

Testing memory access...
PASS: Memory access test

Testing instruction execution...
PASS: Instruction execution test

All simulator tests passed!
```

## Missing Unit Tests

The following components **lack unit tests**:

| Component | What Should Be Tested |
|-----------|----------------------|
| **Instruction Decoder** | Bit extraction, sign extension, opcode decoding |
| **Flag Logic** | N, Z, C, V flag updates after arithmetic |
| **Branch Conditions** | All 16 condition codes (EQ, NE, CS, etc.) |
| **Memory Byte Ordering** | Little-endian halfword/word read/write |
| **NVIC** | Enable/disable, pending/clear, priority |
| **TIM6** | Counter update, overflow, interrupt generation |
| **USART** | Transmit, receive, baud rate calculation |
| **Orchestrator KeyDB client** | PopTask, PushResult, error handling |
| **Orchestrator Runner** | Base64 decode, temp file creation, timeout |
| **Orchestrator Parser** | Regex extraction of stats, GPIO, UART, registers |

## Test Design Principles

1. **Arrange-Act-Assert** — Setup, execute, verify
2. **Isolation** — Each test allocates its own `Simulator` struct
3. **Cleanup** — Every test calls `memory_free()` before returning
4. **Early exit** — Return immediately on failure (no cascade failures)
5. **Descriptive messages** — Include expected vs actual values in failure messages

---

#unit-tests #c-testing #memory-tests #cpu-tests #instruction-tests #gpio-tests
