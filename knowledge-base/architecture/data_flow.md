# Data Flow

> [!summary]
> This document traces the complete data flow through the simulator system, from firmware loading to result extraction, showing how each component interacts during execution.

## Firmware Loading Flow

```
Student/Teacher
     │
     │ 1. Creates .bin file
     │    (Thumb-16 instructions)
     ▼
┌─────────────────────────────┐
│  Option A: CLI (manual)     │
│  stm32_sim firmware.bin     │
└──────────┬──────────────────┘
           │
           │ 2. argv[1] = "firmware.bin"
           ▼
┌─────────────────────────────┐
│  main.c: main()             │
│  run_demo = 0               │
│  firmware_file = argv[1]    │
└──────────┬──────────────────┘
           │
           │ 3. load_binary_file(&sim.mem, filename)
           ▼
┌─────────────────────────────┐
│  fopen("firmware.bin", "rb")│
│  fseek → ftell → file_size  │
│  fread(mem->flash, ...)     │
└──────────┬──────────────────┘
           │
           │ 4. Bytes copied to Flash array
           ▼
┌──────────────────────────────────────┐
│  Memory.flash[] = [                  │
│    0x05, 0x20,  // MOV R0, #5       │
│    0x03, 0x21,  // MOV R1, #3       │
│    0x42, 0x18,  // ADDS R2, R0, R1  │
│    0xFF, 0xE7,  // B .              │
│  ]                                   │
│  PC = 0x08000000                     │
└──────────────────────────────────────┘
```

## Instruction Execution Flow

### Example: `MOV R0, #5` (encoding: `0x2005`)

```
┌─────────────────────────────────────────────┐
│  simulator_step(sim)                         │
│  PC = 0x08000000                             │
└──────────────────┬──────────────────────────┘
                   │
                   │ 1. FETCH
                   ▼
┌─────────────────────────────────────────────┐
│  instr = memory_read_halfword(mem, PC)      │
│  addr = 0x08000000                           │
│  flash[0] = 0x05, flash[1] = 0x20           │
│  instr = 0x05 | (0x20 << 8) = 0x2005        │
│  PC += 2 → 0x08000002                        │
└──────────────────┬──────────────────────────┘
                   │
                   │ 2. DECODE
                   ▼
┌─────────────────────────────────────────────┐
│  opcode = get_bits(instr, 11, 15)           │
│       = (0x2005 >> 11) & 0x1F               │
│       = 0b00001 = 0x01                       │
│                                               │
│  Wait — that doesn't match MOV!              │
│                                               │
│  Actually, MOV #imm8 has opcode 0b00100      │
│  Let's recalculate:                           │
│  0x2005 = 0010 0000 0000 0101               │
│  bits[15:11] = 00100 = 0x04 ✓               │
└──────────────────┬──────────────────────────┘
                   │
                   │ 3. EXECUTE (case 0b00100)
                   ▼
┌─────────────────────────────────────────────┐
│  rd  = get_bits(instr, 8, 10) = 0  → R0    │
│  imm = get_bits(instr, 0, 7) = 5            │
│  cpu->regs[0] = 5                           │
│  update_flags(cpu, 5)                       │
│    Z = 0 (result ≠ 0)                       │
│    N = 0 (bit 31 = 0)                       │
└─────────────────────────────────────────────┘
```

## Memory Access Flow (LDR Instruction)

### Example: `LDR R0, [PC, #imm8]` (encoding: `0x4800`)

```
┌─────────────────────────────────────────────┐
│  Case 0b10110: LDR Rd, [PC, #imm8]          │
│  instr = 0x4802  // LDR R0, [PC, #8]        │
└──────────────────┬──────────────────────────┘
                   │
                   │ 1. Decode fields
                   ▼
┌─────────────────────────────────────────────┐
│  rd  = bits[10:8] = 0      → R0            │
│  imm = bits[7:0] = 2       → 2 × 4 = 8     │
│                                               │
│  addr = (PC & ~0x3) + (imm << 2)           │
│       = (0x08000008 & ~0x3) + (2 << 2)     │
│       = 0x08000008 + 8                      │
│       = 0x08000010                           │
└──────────────────┬──────────────────────────┘
                   │
                   │ 2. Read from Flash
                   ▼
┌─────────────────────────────────────────────┐
│  cpu->regs[0] = memory_read_word(mem, addr) │
│                                               │
│  b0 = flash[0x10] = 0x34                    │
│  b1 = flash[0x11] = 0x12                    │
│  b2 = flash[0x12] = 0x78                    │
│  b3 = flash[0x13] = 0x56                    │
│                                               │
│  result = 0x34 | (0x12<<8) | (0x78<<16)    │
│         | (0x56<<24)                         │
│         = 0x56781234                         │
│                                               │
│  R0 = 0x56781234                              │
└─────────────────────────────────────────────┘
```

## GPIO Write Flow

### Example: `STR R0, [R1, #imm5]` writing to GPIOA_ODR

```
┌─────────────────────────────────────────────┐
│  R0 = 0x00000001  (value to write)          │
│  R1 = 0x4001080C  (GPIOA_ODR address)       │
│  instr = 0x6008  // STR R0, [R1, #0]       │
└──────────────────┬──────────────────────────┘
                   │
                   │ ⚠️ Current limitation:
                   │ Memory writes to peripheral
                   │ region (0x40000000+) are
                   │ NOT routed to GPIO module.
                   │ They are silently ignored.
                   ▼
┌─────────────────────────────────────────────┐
│  memory_write_word(mem, 0x4001080C, 0x01)   │
│                                               │
│  if (addr >= SRAM_BASE && ...) → NO         │
│  // Flash writes ignored                     │
│  // Peripheral writes NOT handled            │
│                                               │
│  → Nothing happens (silent drop)             │
└──────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│  🔧 REQUIRED FIX:                            │
│  In main loop or memory module, add:        │
│                                               │
│  if (addr >= GPIO_BASE && addr < GPIO_END)  │
│      gpio_write_register(gpio, addr, val);  │
│  else if (addr >= TIM6_BASE && ...)         │
│      tim6_write_register(tim6, addr, val);  │
│  ...                                         │
└─────────────────────────────────────────────┘
```

## Interrupt Flow (Conceptual — Not Yet Implemented)

```
TIM6 Counter Overflow
         │
         │ 1. Set update flag
         ▼
┌─────────────────────┐
│  tim6->sr |= UIF    │
└──────────┬──────────┘
           │
           │ 2. If UIE=1, set pending in NVIC
           ▼
┌─────────────────────────────────┐
│  nvic_set_pending(nvic, TIM6_IRQn) │
│  irqn = 22                       │
│  reg = 22 / 32 = 0               │
│  bit = 22 % 32 = 22              │
│  nvic->ispr[0] |= (1 << 22)     │
└──────────┬──────────────────────┘
           │
           │ 3. Check if interrupt should fire
           │    (at end of simulator_step)
           ▼
┌─────────────────────────────────┐
│  if (!nvic_is_pending(nvic, 22)) │
│      return;                     │
│  if (cpu->primask & 1) return;   │  ← Masked
│                                   │
│  priority = nvic_get_priority(22)│
│  if (priority <= cpu->basepri)   │
│      return;                     │  ← Below base priority
└──────────┬──────────────────────┘
           │
           │ 4. Enter exception
           ▼
┌─────────────────────────────────┐
│  // Stacking (push to SP)        │
│  SP -= 8 × 4;  // 32 bytes      │
│  memory_write_word(mem, SP+0, R0); │
│  memory_write_word(mem, SP+4, R1); │
│  ...                              │
│  memory_write_word(mem, SP+24, xPSR);│
│                                   │
│  // Update state                  │
│  cpu->xpsr |= 22;  // IPSR = 22  │
│  cpu->pc = memory_read_word(      │
│      mem, 0x08000000 + 22*4      │
│  );  // Vector table lookup       │
└───────────────────────────────────┘
```

## Orchestrator Flow

```
┌───────────────────────┐
│  task.json:            │
│  {                     │
│    "task_id": "lab1",  │
│    "binary": "BQUDGII=",│  ← base64(.bin)
│    "timeout_sec": 5    │
│  }                     │
└───────────┬───────────┘
            │
            │ orchestrator --mode file --file task.json
            ▼
┌──────────────────────────────────────┐
│  main.go: runFileMode()              │
│  task = json.Unmarshal(inputFile)    │
└───────────┬──────────────────────────┘
            │
            │ runner.RunTask(ctx, task)
            ▼
┌──────────────────────────────────────┐
│  runner.go:                          │
│  1. binaryData = base64.Decode(...)  │
│  2. tmpFile = CreateTemp("*.bin")    │
│  3. tmpFile.Write(binaryData)        │
│  4. cmd = exec.Command(              │
│       "stm32_sim",                   │
│       "--max-steps", "10000",        │
│       tmpFilename                    │
│     )                                │
└───────────┬──────────────────────────┘
            │
            │ Simulator runs, outputs to stdout
            ▼
┌──────────────────────────────────────┐
│  stdout:                              │
│  [INIT] Loaded 8 bytes...            │
│  [CPU] Final register state:         │
│    R0:  0x00000005                   │
│    R1:  0x00000003                   │
│    R2:  0x00000008                   │
│  [STATS] Instructions executed: 4    │
└───────────┬──────────────────────────┘
            │
            │ parseOutput(result, stdout)
            ▼
┌──────────────────────────────────────┐
│  result.InstructionsExecuted = 4     │
│  result.GPIOState["R0"] = 5          │
│  result.GPIOState["R1"] = 3          │
│  result.GPIOState["R2"] = 8          │
│  result.Status = "success"           │
└───────────┬──────────────────────────┘
            │
            │ json.Marshal(result) → result.json
            ▼
┌──────────────────────────────────────┐
│  result.json:                         │
│  {                                    │
│    "task_id": "lab1",                │
│    "status": "success",              │
│    "gpio_state": {                   │
│      "R0": 5,                        │
│      "R1": 3,                        │
│      "R2": 8                         │
│    },                                │
│    "instructions_executed": 4        │
│  }                                    │
└──────────────────────────────────────┘
```

## Component Interaction Matrix

| From → To | Mechanism | Data |
|-----------|-----------|------|
| **File → main.c** | `fread()` | Raw bytes to Flash |
| **main.c → CPU** | `cpu_reset()`, `PC = FLASH_BASE` | Initial state |
| **CPU → Memory** | `memory_read_halfword(PC)` | Instruction fetch |
| **CPU → Memory** | `memory_read_word(addr)` | LDR instruction |
| **CPU → Memory** | `memory_write_word(addr, val)` | STR instruction |
| **Memory → GPIO** | ❌ Not routed | Writes dropped |
| **TIM6 → NVIC** | ❌ Not implemented | No interrupt signaling |
| **Simulator → stdout** | `printf()` | Log lines |
| **stdout → Orchestrator** | Regex parsing | Structured results |

---

#data-flow #execution #fetch-decode-execute #memory-access #interrupts #orchestrator
