# Integration Tests

> [!summary]
> Integration tests verify correct interaction between subsystem components, ensuring that data flows correctly across module boundaries.

## Integration Test Categories

### 1. CPU ↔ Memory Integration

**Test:** Firmware loading and execution

**Scenario:**
```c
// Arrange
Simulator sim;
memory_init(&sim.mem);
cpu_reset(&sim.cpu);

// Load program: MOV R0,#5; ADDS R1,R0,#3; B .
uint16_t program[] = {0x2005, 0x3083, 0xE7FE};
memcpy(sim.mem.flash, program, sizeof(program));
sim.cpu.pc = FLASH_BASE_ADDR;

// Act — execute 3 steps
simulator_step(&sim);  // MOV R0,#5
simulator_step(&sim);  // ADDS R1,R0,#3
simulator_step(&sim);  // B . (loops back)

// Assert
assert(sim.cpu.regs[0] == 5);
assert(sim.cpu.regs[1] == 8);
assert(sim.cpu.pc == FLASH_BASE_ADDR + 4);  // After branch
```

**What's verified:**
- Instruction fetch from Flash
- PC advancement (PC += 2 per instruction)
- Register read/write across steps
- Branch target calculation

### 2. CPU ↔ GPIO Integration (Conceptual — Not Yet Working)

**Test:** STR to GPIO register

**Scenario:**
```c
// Arrange
Simulator sim;
memory_init(&sim.mem);
cpu_reset(&sim.cpu);
gpio_init(&sim.gpio);

// Load: MOV R0,#0xFF; MOV R1,GPIOA_ODR; STR R0,[R1]
uint16_t program[] = {
    0x20FF,       // MOV R0,#0xFF
    0x4902,       // LDR R1,[PC,#8]  → load GPIOA_ODR address
    0x6008,       // STR R0,[R1,#0]
    0xE7FE,       // B .
    0x0000,       // padding
    0x0000,       // padding
    0x080C,       // low half of GPIOA_ODR (0x4001080C)
    0x4001,       // high half
};

// Act
simulator_run(&sim, 10);

// Assert (requires bus routing fix)
assert(sim.gpio.ports[0].odr == 0xFF);
```

**What should be verified:**
- LDR [PC, #imm] loads literal pool address
- STR writes to peripheral address
- Memory routes write to GPIO module
- GPIO ODR updated and logged

**Current status:** ❌ Fails because `memory_write_byte` drops peripheral writes

### 3. TIM6 ↔ NVIC Integration (Conceptual)

**Test:** Timer overflow generates interrupt

**Scenario:**
```c
// Arrange
TIM6_State tim6;
NVIC_State nvic;
tim6_init(&tim6);
nvic_init(&nvic);

// Configure timer: ARR=5, enable update interrupt
tim6.regs.arr = 5;
tim6.regs.cr1 = TIM6_CR1_CEN;  // Enable counter
tim6.regs.dier = TIM6_DIER_UIE;  // Enable update interrupt

// Act — simulate 6 counter updates
for (int i = 0; i < 6; i++) {
    tim6_update_counter(&tim6);
}

// Assert
assert(tim6.regs.sr & TIM6_SR_UIF);  // UIF flag set
assert(nvic_is_pending(&nvic, 22));   // TIM6 IRQ pending
```

**What should be verified:**
- Counter increments on each update call
- Counter wraps at ARR value
- UIF flag set on overflow
- NVIC pending bit set when UIE=1

**Current status:** ❌ Fails because `tim6_update_counter` doesn't call NVIC

### 4. Orchestrator ↔ Simulator Integration

**Test:** File mode execution

**Scenario:**
```bash
# Arrange
cat > test_task.json << 'EOF'
{
  "task_id": "integration_test_1",
  "student_id": "test",
  "lab_number": 1,
  "binary": "BQUDGII=",
  "timeout_sec": 5,
  "config": {}
}
EOF

# Act
./orchestrator --mode file \
    --file test_task.json \
    --output result.json \
    --simulator ../build/stm32_sim

# Assert
cat result.json | jq '.status'          # → "success"
cat result.json | jq '.gpio_state.R0'   # → 5
cat result.json | jq '.gpio_state.R1'   # → 3
cat result.json | jq '.gpio_state.R2'   # → 8
```

**What's verified:**
- JSON parsing (task loading)
- Base64 decoding (binary extraction)
- Temp file creation
- Process spawning
- Simulator execution
- stdout capture
- Regex parsing (register extraction)
- JSON output (result serialization)

### 5. KeyDB ↔ Orchestrator Integration (Requires Redis)

**Test:** Queue mode task processing

**Scenario:**
```bash
# Arrange — start Redis
docker run -d -p 6379:6379 redis:latest

# Push task to queue
redis-cli LPUSH simulator:tasks '{
  "task_id": "queue_test_1",
  "student_id": "student_42",
  "lab_number": 1,
  "binary": "BQUDGII=",
  "timeout_sec": 5
}'

# Act — run orchestrator
./orchestrator --mode queue --redis localhost:6379

# Assert — check result
redis-cli RPOP simulator:results
# → {"task_id":"queue_test_1","status":"success","gpio_state":{"R0":5,...}}
```

**What's verified:**
- Redis connection
- BLPOP task extraction
- Task deserialization
- Result serialization
- RPUSH result pushing

## Integration Test Matrix

| Test | CPU | Memory | GPIO | TIM6 | NVIC | USART | Orchestrator | Status |
|------|-----|--------|------|------|------|-------|--------------|--------|
| Load + execute | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ Working |
| GPIO register write | ✅ | ⚠️ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ Broken |
| Timer overflow | ✅ | ❌ | ❌ | ✅ | ✅ | ❌ | ❌ | ❌ Not implemented |
| UART transmit | ✅ | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ | ❌ Not implemented |
| Orchestrator file mode | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ✅ | ✅ Working |
| Orchestrator queue mode | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ✅ | ⚠️ Requires Redis |

## Running Integration Tests

### Automated (File Mode)

```bash
# Generate all demo binaries
python3 demo_programs.py all

# Run each through orchestrator
for bin in arithmetic.bin fibonacci.bin max_value.bin; do
    echo "Testing $bin..."
    ./orchestrator --mode file \
        --file <(echo "{\"task_id\":\"test\",\"binary\":\"$(base64 -w0 $bin)\",\"timeout_sec\":5}") \
        --output result.json \
        --simulator ./build/stm32_sim
    
    # Verify result
    jq '.status' result.json  # Should be "success"
done
```

### Manual (Interactive)

```bash
# Run simulator directly with demo
./build/stm32_sim --demo --max-steps 10

# Check output format
./build/stm32_sim fibonacci.bin --max-steps 50 | grep -E '\[STATS\]|R0:|R1:'
```

## Integration Test Gaps

| Gap | Root Cause | Required Fix |
|-----|-----------|--------------|
| GPIO writes don't reach GPIO module | `memory_write_byte` drops peripheral writes | Add peripheral routing in memory module |
| TIM6 doesn't generate interrupts | `tim6_update_counter` doesn't call NVIC | Add NVIC integration in TIM6 |
| USART doesn't output characters | DR write doesn't trigger transmission | Add transmit logic in USART |
| No interrupt entry/exit | CPU doesn't handle exceptions | Implement stacking/unstacking, vector lookup |

---

#integration-tests #subsystem-testing #cpu-memory #gpio #timer #orchestrator
