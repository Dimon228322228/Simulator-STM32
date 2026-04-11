# System Tests

> [!summary]
> System tests verify end-to-end execution of complete student workflows, from firmware compilation through simulator execution to result verification.

## System Test Architecture

System tests operate at the **highest level** of the testing pyramid:

```
Student submits .bin
       │
       ▼
Orchestrator receives task
       │
       ▼
Simulator executes firmware
       │
       ▼
Output parsed → Results returned
       │
       ▼
Results compared against expected values
```

## Test Scenarios

### 1. Arithmetic Operations

**Firmware:** `arithmetic.bin`

**Source code (conceptual assembly):**
```assembly
    MOV R0, #10      @ R0 = 10
    MOV R1, #3       @ R1 = 3
    ADDS R2, R0, R1  @ R2 = 10 + 3 = 13
    SUBS R3, R0, R1  @ R3 = 10 - 3 = 7
    CMP R0, R1       @ Compare 10 vs 3
    BGT greater      @ Branch if R0 > R1
    MOV R4, #0       @ R4 = 0 (not taken)
    B end
greater:
    MOV R4, #1       @ R4 = 1 (taken)
end:
    B .              @ Infinite loop
```

**Binary generation (Python):**
```python
instructions = [
    0x200A,  # MOV R0, #10
    0x2103,  # MOV R1, #3
    0x1842,  # ADDS R2, R0, R1  → R2=13
    0x1A43,  # SUBS R3, R0, R1  → R3=7
    0x4288,  # CMP R0, R1
    0xDC02,  # BGT +2 (skip next instruction)
    0x2400,  # MOV R4, #0 (not taken)
    0xE001,  # B +2 (skip to end)
    0x2401,  # MOV R4, #1 (taken)
    0xE7FE,  # B . (infinite loop)
]
```

**Expected results:**
| Register | Expected Value | Meaning |
|----------|---------------|---------|
| R0 | 10 | First operand |
| R1 | 3 | Second operand |
| R2 | 13 | Addition result |
| R3 | 7 | Subtraction result |
| R4 | 1 | Branch taken (R0 > R1) |

**Test command:**
```bash
./build/stm32_sim arithmetic.bin --max-steps 20
```

**Verification:**
```bash
./build/stm32_sim arithmetic.bin --max-steps 20 | \
  grep -E 'R0:.*0x0000000A.*R1:.*0x00000003.*R2:.*0x0000000D'
```

### 2. Fibonacci Sequence

**Firmware:** `fibonacci.bin`

**Algorithm:**
```
R0 = 0  (F(n-2))
R1 = 1  (F(n-1))
R2 = 8  (iteration count)

loop:
  R3 = R0 + R1  (F(n))
  R0 = R1       (shift)
  R1 = R3       (shift)
  R2--          (decrement counter)
  if R2 > 0: goto loop
```

**Expected results after 8 iterations:**
| Iteration | R0 (F(n-2)) | R1 (F(n-1)) |
|-----------|-------------|-------------|
| 0 | 0 | 1 |
| 1 | 1 | 1 |
| 2 | 1 | 2 |
| 3 | 2 | 3 |
| 4 | 3 | 5 |
| 5 | 5 | 8 |
| 6 | 8 | 13 |
| 7 | 13 | 21 |
| 8 | **21** | **34** |

**Test command:**
```bash
./build/stm32_sim fibonacci.bin --max-steps 50
```

**Verification:**
```bash
output=$(./build/stm32_sim fibonacci.bin --max-steps 50)
echo "$output" | grep "R0:  0x00000015"  # 21 in hex
echo "$output" | grep "R1:  0x00000022"  # 34 in hex
```

### 3. LED Blink Simulation

**Firmware:** `led_blink.bin`

**Algorithm:**
```
R0 = 0       (GPIO state)
R1 = 5       (blink count)
R2 = 0       (counter)

loop:
  EOR R0, R0, #1   @ Toggle bit 0
  R2++
  if R2 < R1: goto loop
```

**Expected behavior:**
| Step | R0 (LED state) |
|------|----------------|
| 0 | 0 |
| 1 | 1 |
| 2 | 0 |
| 3 | 1 |
| 4 | 0 |
| 5 | 1 |

**Expected results:**
| Register | Expected Value |
|----------|---------------|
| R0 | 1 (toggled 5 times, ends at 1) |
| R1 | 5 (blink count) |
| R2 | 5 (loop counter) |

**System test via orchestrator:**
```bash
./orchestrator --mode file \
    --file <(echo '{"task_id":"blink","binary":"'"$(base64 -w0 led_blink.bin)"'","timeout_sec":5}') \
    --output result.json \
    --simulator ./build/stm32_sim

jq '.gpio_state.R0' result.json  # → 1
```

### 4. Maximum Value Selection

**Firmware:** `max_value.bin`

**Algorithm:**
```
R0 = 42      @ Value A
R1 = 17      @ Value B
CMP R0, R1
BGT a_is_max
MOV R2, R1   @ B is max
B end
a_is_max:
MOV R2, R0   @ A is max
end:
B .
```

**Expected results:**
| Register | Expected Value |
|----------|---------------|
| R0 | 42 |
| R1 | 17 |
| R2 | 42 (maximum) |

### 5. Memory Load from Flash

**Firmware:** `memory.bin`

**Algorithm:**
```assembly
    LDR R0, [PC, #8]   @ Load constant from literal pool
    LDR R1, [PC, #8]   @ Load another constant
    ADDS R2, R0, R1    @ Add them
    B .
    
    @ Literal pool:
    .word 0xDEAD
    .word 0xBEEF
```

**Expected results:**
| Register | Expected Value |
|----------|---------------|
| R0 | 0xDEAD |
| R1 | 0xBEEF |
| R2 | 0xDEAD + 0xBEEF = 0x19D9C |

## System Test Runner Script

### Linux (`run_system_tests.sh`)

```bash
#!/bin/bash
set -e

SIMULATOR="./build/stm32_sim"
PASS=0
FAIL=0

run_test() {
    local name=$1
    local binary=$2
    local max_steps=$3
    local expected_register=$4
    local expected_value=$5
    
    echo -n "Testing $name... "
    
    output=$($SIMULATOR "$binary" --max-steps "$max_steps" 2>&1)
    
    # Extract register value
    actual_value=$(echo "$output" | grep "$expected_register:" | \
        sed -E "s/.*$expected_register:\s+0x([0-9A-Fa-f]+).*/\1/")
    
    # Convert expected value to hex
    expected_hex=$(printf "%08X" "$expected_value")
    
    if [ "$actual_value" = "$expected_hex" ]; then
        echo "PASS"
        ((PASS++))
    else
        echo "FAIL (expected $expected_hex, got $actual_value)"
        ((FAIL++))
    fi
}

# Generate test binaries
python3 demo_programs.py all

# Run tests
run_test "Arithmetic" "arithmetic.bin" 20 "R2" 13
run_test "Fibonacci" "fibonacci.bin" 50 "R0" 21
run_test "LED Blink" "led_blink.bin" 20 "R0" 1
run_test "Max Value" "max_value.bin" 20 "R2" 42
run_test "Memory Load" "memory.bin" 20 "R2" 105884  # 0x19D9C

echo ""
echo "Results: $PASS passed, $FAIL failed"

if [ $FAIL -gt 0 ]; then
    exit 1
fi
```

## System Test via Orchestrator

### Task JSON Template

```json
{
  "task_id": "system_test_fibonacci",
  "student_id": "test_suite",
  "lab_number": 2,
  "binary": "<base64-encoded fibonacci.bin>",
  "timeout_sec": 5,
  "config": {}
}
```

### Result Verification

```bash
# Run orchestrator
./orchestrator --mode file \
    --file system_test_fibonacci.json \
    --output result.json \
    --simulator ./build/stm32_sim

# Verify result structure
jq -e '.status == "success"' result.json
jq -e '.gpio_state.R0 == 21' result.json
jq -e '.gpio_state.R1 == 34' result.json
jq -e '.instructions_executed > 0' result.json
```

## Performance Tests

### Execution Speed

| Firmware | Steps | Real Time (ms) | Steps/sec |
|----------|-------|----------------|-----------|
| arithmetic.bin | 10 | <10 | >1000 |
| fibonacci.bin | 50 | <50 | >1000 |
| led_blink.bin | 20 | <10 | >2000 |

**Requirement:** "Типовое задание должно выполняться не дольше 2 секунд реального времени."

### Memory Usage

| Component | Memory (MB) |
|-----------|-------------|
| Simulator process | <5 |
| Orchestrator | <20 |
| Total per task | <25 |

**Requirement:** "Один экземпляр ядра — не более 50 МБ ОЗУ."

## System Test Checklist

- [ ] All demo binaries generate successfully
- [ ] Each binary executes without errors
- [ ] Register values match expected results
- [ ] Statistics are reported correctly
- [ ] No memory leaks (valgrind clean)
- [ ] Orchestrator parses all output fields
- [ ] Timeout handling works (infinite loop terminates)
- [ ] Error handling works (invalid binary produces error message)

---

#system-tests #end-to-end #demo-programs #verification #performance
