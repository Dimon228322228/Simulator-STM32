# Test Infrastructure

> [!summary]
> The test infrastructure encompasses build systems, test generation scripts, execution scripts, and result verification tools.

## Build System

### CMakeLists.txt

The project uses CMake for building both the simulator and test executables:

```cmake
cmake_minimum_required(VERSION 3.10)
project(STM32_Simulator C Go)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(core/include)

# Simulator library (core components)
add_library(simulator_core
    core/src/memory.c
    core/src/cpu_state.c
    core/src/execute.c
    core/src/gpio.c
    # ... other peripheral sources
)

# Main simulator executable
add_executable(stm32_sim core/main.c)
target_link_libraries(stm32_sim simulator_core)

# Test executables
add_executable(test_simulator test/test_simulator.c)
target_link_libraries(test_simulator simulator_core)

add_executable(test_gpio test/test_gpio.c)
target_link_libraries(test_gpio simulator_core)
```

### Build Commands

```bash
# Configure
mkdir build && cd build
cmake ..

# Build all targets
make

# Build specific target
make stm32_sim
make test_simulator

# Verbose build (see compiler commands)
make VERBOSE=1
```

## Test Binary Generation

### `create_test_bin.py`

Generates `.bin` files from instruction arrays:

```python
#!/usr/bin/env python3
import struct
import sys
import base64

def create_binary(filename, instructions, name="test"):
    """Create a Thumb-16 binary file from instruction list"""
    with open(filename, "wb") as f:
        for instr in instructions:
            f.write(struct.pack("<H", instr))  # Little-endian halfword
    
    size = len(instructions) * 2
    b64 = base64.b64encode(bytes(instr for i in instructions for instr in struct.pack("<H", i))).decode()
    
    print(f"Created {filename} ({size} bytes)")
    print(f"  Base64: {b64}")
    return filename

def create_demo():
    """Simple demo: R0=5, R1=3, R2=8"""
    create_binary("demo.bin", [
        0x2005,  # MOV R0, #5
        0x2103,  # MOV R1, #3
        0x1842,  # ADDS R2, R0, R1
        0xE7FF,  # B .
    ], "demo")

def create_all():
    """Generate all test binaries"""
    create_demo()
    # ... more test binaries

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "all":
        create_all()
    else:
        create_demo()
```

### `demo_programs.py`

Generates all demo binaries for comprehensive testing:

```python
#!/usr/bin/env python3
import struct
import sys

def write_bin(filename, instructions):
    """Write instruction list to binary file"""
    with open(filename, "wb") as f:
        for instr in instructions:
            f.write(struct.pack("<H", instr))

def arithmetic():
    """Arithmetic operations with conditional branch"""
    write_bin("arithmetic.bin", [
        0x200A,  # MOV R0, #10
        0x2103,  # MOV R1, #3
        0x1842,  # ADDS R2, R0, R1  → R2=13
        0x1A43,  # SUBS R3, R0, R1  → R3=7
        0xE7FE,  # B .
    ])

def fibonacci():
    """Calculate Fibonacci sequence (8 iterations)"""
    write_bin("fibonacci.bin", [
        0x2000,  # MOV R0, #0    (F(n-2))
        0x2101,  # MOV R1, #1    (F(n-1))
        0x2208,  # MOV R2, #8    (iterations)
        0x1843,  # ADDS R3, R0, R1  (F(n) = F(n-2) + F(n-1))
        0x4608,  # MOV R0, R1    (shift: R0 = F(n-1))
        0x4619,  # MOV R1, R3    (shift: R1 = F(n))
        0x3A01,  # SUBS R2, #1   (decrement counter)
        0x2A00,  # CMP R2, #0
        0xD1F9,  # BNE loop (if R2 != 0)
        0xE7FE,  # B .
    ])

def led_blink():
    """Toggle LED (XOR) in loop"""
    write_bin("led_blink.bin", [
        0x2000,  # MOV R0, #0    (LED state)
        0x2105,  # MOV R1, #5    (blink count)
        0x2200,  # MOV R2, #0    (counter)
        0x4048,  # EORS R0, R1   (toggle - conceptual)
        0x3201,  # ADDS R2, #1
        0x4291,  # CMP R2, R1
        0xD3F9,  # BCC loop
        0xE7FE,  # B .
    ])

def max_value():
    """Find maximum of two values"""
    write_bin("max_value.bin", [
        0x202A,  # MOV R0, #42
        0x2111,  # MOV R1, #17
        0x4288,  # CMP R0, R1
        0xDC02,  # BGT +2 (a_is_max)
        0x460A,  # MOV R2, R1
        0xE001,  # B end
        0x4602,  # MOV R2, R0
        0xE7FE,  # B .
    ])

def memory_load():
    """Load constants from Flash via PC-relative LDR"""
    write_bin("memory.bin", [
        0x4802,  # LDR R0, [PC, #8]  → 0xDEAD
        0x4902,  # LDR R1, [PC, #8]  → 0xBEEF
        0x1842,  # ADDS R2, R0, R1
        0xE7FE,  # B .
        0xDEAD,  # Literal: 0xDEAD (low half)
        0x0000,  # Literal: 0xDEAD (high half, padding)
        0xBEEF,  # Literal: 0xBEEF (low half)
        0x0000,  # Literal: 0xBEEF (high half, padding)
    ])

def generate_all():
    """Generate all demo binaries"""
    arithmetic()
    fibonacci()
    led_blink()
    max_value()
    memory_load()
    print("Generated all demo binaries")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "all":
        generate_all()
    else:
        print("Usage: python3 demo_programs.py all")
```

## Test Execution Scripts

### Linux (`run_tests.sh`)

```bash
#!/bin/bash
set -e

echo "Building project..."
mkdir -p build && cd build
cmake ..
make

echo ""
echo "Running tests..."
echo "=================="

# Run C tests
./test_gpio
./test_simulator

echo ""
echo "All tests passed!"
```

### Windows (`run_tests.ps1`)

```powershell
$ErrorActionPreference = "Stop"

Write-Host "Building project..." -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path build | Out-Null
Set-Location build
cmake .. -G "MinGW Makefiles"
mingw32-make

Write-Host ""
Write-Host "Running tests..." -ForegroundColor Cyan
Write-Host "==================" -ForegroundColor Cyan

# Run C tests
.\test_gpio.exe
.\test_simulator.exe

Write-Host ""
Write-Host "All tests passed!" -ForegroundColor Green
```

### Quick Test (`quick_test.sh` / `quick_test.ps1`)

Minimal verification that the simulator builds and runs:

```bash
#!/bin/bash
# Quick verification
mkdir -p build && cd build
cmake .. >/dev/null 2>&1
make >/dev/null 2>&1
./stm32_sim --demo --max-steps 5
```

## Test Task Definition

### `test_task.json`

Example task for orchestrator testing:

```json
{
  "task_id": "demo_task_001",
  "student_id": "test_user",
  "lab_number": 1,
  "binary": "BQUDGII=",
  "timeout_sec": 5,
  "config": {
    "gpio_input": "",
    "uart_input": ""
  }
}
```

**Binary breakdown:**
```
BQUDGII= → base64 decode → [0x05, 0x20, 0x03, 0x21, 0x42, 0x18, 0xFF, 0xE7]
         → Instructions:
            0x2005  MOV R0, #5
            0x2103  MOV R1, #3
            0x1842  ADDS R2, R0, R1
            0xE7FF  B .
```

## Result Verification Tools

### Manual Verification

```bash
# Run simulator, extract register values
output=$(./build/stm32_sim fibonacci.bin --max-steps 50)
r0=$(echo "$output" | grep "R0:" | sed -E 's/.*R0:\s+0x([0-9A-F]+).*/\1/')
echo "R0 = 0x$r0 = $((16#$r0))"
```

### Automated Verification (JQ)

```bash
# Run orchestrator, verify with jq
./orchestrator --mode file --file task.json --output result.json

jq -e '.status == "success"' result.json && echo "✅ Status OK"
jq -e '.gpio_state.R0 == 21' result.json && echo "✅ R0 correct"
jq -e '.gpio_state.R1 == 34' result.json && echo "✅ R1 correct"
jq -e '.instructions_executed > 0' result.json && echo "✅ Instructions executed"
```

### Diff-Based Verification

```bash
# Capture expected output
./build/stm32_sim demo.bin --max-steps 10 > expected.txt 2>&1

# Run again, compare
./build/stm32_sim demo.bin --max-steps 10 > actual.txt 2>&1
diff expected.txt actual.txt && echo "✅ Outputs match"
```

## Continuous Integration

### GitHub Actions Workflow

```yaml
name: STM32 Simulator Tests

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install Dependencies
      run: |
        sudo apt update
        sudo apt install -y build-essential cmake python3 golang-go
    
    - name: Build Simulator
      run: |
        mkdir -p build && cd build
        cmake ..
        make
    
    - name: Run Unit Tests
      run: |
        cd build
        ./test_gpio
        ./test_simulator
    
    - name: Generate Demo Binaries
      run: python3 demo_programs.py all
    
    - name: Run System Tests
      run: |
        cd build
        ./stm32_sim ../arithmetic.bin --max-steps 20
        ./stm32_sim ../fibonacci.bin --max-steps 50
    
    - name: Build Orchestrator
      run: |
        cd orchestrator
        go mod download
        go build
    
    - name: Test Orchestrator (File Mode)
      run: |
        echo '{"task_id":"ci_test","binary":"BQUDGII=","timeout_sec":5}' > task.json
        ./orchestrator --mode file --file task.json --output result.json
        jq -e '.status == "success"' result.json
```

## Test Data Management

### Binary File Lifecycle

```
create_test_bin.py / demo_programs.py
         │
         ▼
    .bin files (generated per run)
         │
         ├──► Direct simulator execution
         │         │
         │         ▼
         │    stdout output
         │
         ├──► Orchestrator (base64 encoded in JSON)
         │         │
         │         ▼
         │    Temp file → Simulator → Parsed result
         │
         └──► Git (ignored, regenerated on demand)
```

### Cleanup

Test binaries and temp files should be cleaned after testing:

```bash
# Clean build artifacts
rm -rf build/

# Clean generated binaries
rm -f *.bin

# Clean orchestrator temp files (handled automatically by Go defer)
# /tmp/firmware_*.bin → removed after execution
```

---

#test-infrastructure #cmake #python #test-generation #ci-cd #automation #verification
