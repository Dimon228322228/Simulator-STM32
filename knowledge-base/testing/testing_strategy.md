# Testing Strategy

> [!summary]
> The testing strategy for the STM32 simulator follows a multi-level approach: unit tests for individual components, integration tests for subsystem interactions, and system tests for end-to-end verification.

## Testing Pyramid

```
         ┌───────┐
         │ System│  ← Few tests, full stack
         ├───────┤
         │Integration│  ← Medium tests, component pairs
         ├───────────┤
         │   Unit    │  ← Many tests, isolated functions
         └───────────┘
```

## Test Levels

### Unit Tests

**Scope:** Individual functions and modules

| Module | Test File | What's Tested |
|--------|-----------|---------------|
| Memory | `test/test_simulator.c::test_memory_access` | Allocation, read/write, byte ordering |
| CPU State | `test/test_simulator.c::test_simulator_basic` | Reset, register initialization |
| Instruction Execution | `test/test_simulator.c::test_instruction_execution` | MOV, ADD, SUB, B |
| GPIO | `test/test_gpio.c` | Register read/write, BSRR, ODR logging |

**Example:**
```c
int test_instruction_execution() {
    Simulator sim;
    memory_init(&sim.mem);
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);
    
    // Load: MOV R0, #5
    uint16_t program[] = {0x2005};
    memcpy(sim.mem.flash, program, sizeof(program));
    sim.cpu.pc = FLASH_BASE_ADDR;
    
    simulator_step(&sim);
    
    if (sim.cpu.regs[0] != 5) {
        printf("FAIL: Expected R0=5, got R0=%u\n", sim.cpu.regs[0]);
        return 1;
    }
    
    printf("PASS: Instruction execution test\n");
    return 0;
}
```

### Integration Tests

**Scope:** Component interactions

| Test Scenario | Components | What's Verified |
|--------------|------------|-----------------|
| Load firmware → execute | main.c + memory.c + execute.c | Binary loading, instruction fetch, execution |
| GPIO register access | execute.c + gpio.c | STR to GPIO address updates ODR |
| Timer overflow → NVIC | tim6.c + nvic.c | Counter increment sets pending flag |
| Orchestrator → simulator | runner.go + stm32_sim | Process spawn, output parsing |

### System Tests

**Scope:** End-to-end task execution

| Test | Input | Expected Output |
|------|-------|-----------------|
| Arithmetic demo | `fibonacci.bin` | R0=21, R1=34 |
| LED blink | `led_blink.bin` | PORT_A_ODR toggles |
| UART output | `uart_hello.bin` | UARTOutput = "Hello" |
| Infinite loop | `loop.bin` with --max-steps 10 | Stops at step 10 |

## Test Infrastructure

### Build System (CMakeLists.txt)

```cmake
# Test executables
add_executable(test_simulator test/test_simulator.c ...)
add_executable(test_gpio test/test_gpio.c ...)

# Link with simulator libraries
target_link_libraries(test_simulator simulator_core)
```

### Python Test Binary Generator

`create_test_bin.py` generates `.bin` files for testing:

```python
#!/usr/bin/env python3
import struct

def create_demo_bin():
    """Create a simple demo program in Thumb-16"""
    instructions = [
        0x2005,  # MOV R0, #5
        0x2103,  # MOV R1, #3
        0x1842,  # ADDS R2, R0, R1
        0xE7FF,  # B .
    ]
    
    with open("demo.bin", "wb") as f:
        for instr in instructions:
            f.write(struct.pack("<H", instr))  # Little-endian halfword

if __name__ == "__main__":
    create_demo_bin()
    print("Created demo.bin")
```

### Shell Test Runners

**Linux (`run_tests.sh`):**
```bash
#!/bin/bash
cd build
./test_gpio
./test_simulator
```

**Windows (`run_tests.ps1`):**
```powershell
Set-Location build
.\test_gpio.exe
.\test_simulator.exe
```

## Test Coverage Goals

| Component | Target Coverage | Current Status |
|-----------|----------------|----------------|
| Memory | 90%+ | ✅ Allocation, read/write tested |
| CPU State | 90%+ | ✅ Reset tested |
| Instruction Execution | 70%+ | ⚠️ Only MOV, ADD tested |
| GPIO | 70%+ | ⚠️ Register access tested, bus routing not |
| TIM6 | 30% | ❌ Only init tested |
| NVIC | 30% | ❌ Only init tested |
| USART | 30% | ❌ Only init tested |
| Orchestrator | 50% | ⚠️ File mode tested, queue mode not |

## Test Categories

### Positive Tests

Verify correct behavior with valid inputs:

```c
// Test: ADDS R2, R0, R1
// Given: R0=5, R1=3
// When: Execute 0x1842
// Then: R2=8, Z=0, N=0, C=0
```

### Negative Tests

Verify error handling with invalid inputs:

```c
// Test: Unknown instruction
// Given: Flash contains 0xFFFF
// When: Execute
// Then: PC = 0xFFFFFFFF, error message printed
```

### Boundary Tests

Verify edge cases:

| Test | Description |
|------|-------------|
| Flash boundary | Write to `0x0800FFFF` (last byte) |
| SRAM boundary | Write to `0x20004FFF` (last byte) |
| Branch boundary | Branch to first/last Flash address |
| Step limit | Execute exactly `max_steps` instructions |
| Timeout | Orchestrator kills simulator after timeout |

## Continuous Integration

### GitHub Actions Workflow (conceptual)

```yaml
name: Tests
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install dependencies
        run: sudo apt install -y build-essential cmake python3 golang-go
      
      - name: Build simulator
        run: mkdir build && cd build && cmake .. && make
      
      - name: Run C tests
        run: cd build && ./test_gpio && ./test_simulator
      
      - name: Generate test binaries
        run: python3 create_test_bin.py all
      
      - name: Run demo tests
        run: |
          ./build/stm32_sim demo.bin --max-steps 10
          ./build/stm32_sim fibonacci.bin --max-steps 50
      
      - name: Build orchestrator
        run: cd orchestrator && go mod download && go build
      
      - name: Run orchestrator tests
        run: cd orchestrator && go test ./...
```

## Test Output Convention

All tests follow a standard output format:

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

**Failure format:**
```
Testing instruction execution...
FAIL: Instruction execution test failed. Expected R0=5, got R0=0
```

## Test Execution Commands

```bash
# Run all C tests
./build/test_gpio
./build/test_simulator

# Run Python binary generator
python3 create_test_bin.py all
python3 demo_programs.py all

# Run simulator with test binary
./build/stm32_sim demo.bin --max-steps 20

# Run orchestrator in file mode
./orchestrator --mode file --file test_task.json --output result.json

# Quick verification script
./quick_test.sh        # Linux
.\quick_test.ps1       # Windows
```

---

#testing #unit-tests #integration-tests #system-tests #test-strategy #coverage #ci-cd
