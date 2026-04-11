# Architectural Decisions

> [!summary]
> Decision records documenting key architectural choices, alternatives considered, and rationale for the STM32 simulator project.

---

## ADR-001: Two-Tier Architecture (C Simulator + Go Orchestrator)

**Date:** March 2026  
**Status:** Accepted

### Problem

How to build a simulator that can both run locally for development and integrate with a cloud laboratory (ITMO.clab) for automated student assignment evaluation?

### Options

| Option | Pros | Cons |
|--------|------|------|
| **A: Single monolithic C program** | Simple, fast | No queue integration, hard to scale |
| **B: Single monolithic Go program** | Easy queue integration, concurrency | Hard to model low-level memory/CPU in GC language |
| **C: C simulator + Go orchestrator** (chosen) | Accurate low-level modeling + easy cloud integration | Two languages, IPC overhead |
| **D: C simulator with embedded HTTP** | Single binary | Reinvent queue management, no Go concurrency |

### Decision

**Option C: C simulator + Go orchestrator**

### Rationale

1. **C for simulation**: Memory manipulation, bit-level operations, and pointer arithmetic are natural in C. No garbage collector interference with timing-sensitive operations.
2. **Go for orchestration**: Built-in concurrency, excellent Redis client libraries, JSON support, and microservice ecosystem.
3. **Separation of concerns**: The C simulator can be used standalone for development/testing. The orchestrator adds cloud integration without modifying the core.
4. **Educational value**: Students learn both systems programming (C) and cloud-native patterns (Go, Redis, containers).

### Consequences

- **Positive**: Clean architecture, each component can evolve independently
- **Negative**: IPC via stdout parsing is fragile (regex-based)
- **Mitigation**: Structured output format with `[TAG]` prefixes for reliable parsing

---

## ADR-002: Process-per-Task Isolation

**Date:** March 2026  
**Status:** Accepted

### Problem

How to execute multiple student firmware images safely without cross-task interference?

### Options

| Option | Pros | Cons |
|--------|------|------|
| **A: Single process, reset state** | Fast (no process creation overhead) | Risk of state leakage, harder to kill hung tasks |
| **B: New process per task** (chosen) | Complete isolation, easy timeout/kill | Process creation overhead (~10-50ms) |
| **C: Goroutine with shared simulator** | Very fast, concurrent | Requires thread-safe simulator, complex locking |

### Decision

**Option B: New process per task**

### Rationale

1. **Safety**: Student code may contain bugs (infinite loops, invalid memory access). Process isolation ensures one bad task doesn't affect others.
2. **Simplicity**: No need for simulator reset logic (just start fresh).
3. **Timeout enforcement**: `exec.CommandContext` + `Process.Kill()` is reliable and simple.
4. **Resource cleanup**: OS reclaims all resources when process terminates.

### Consequences

- **Positive**: No state leakage, easy to reason about, crash-proof
- **Negative**: ~10-50ms overhead per task (acceptable for 5-second timeout)
- **Future**: If throughput becomes critical, consider process pooling (warm processes ready to execute)

---

## ADR-003: Step-Limit Instead of Real-Time Clock

**Date:** March 2026  
**Status:** Accepted

### Problem

How to model time in the simulator? Real-time clocking would require host-timer synchronization and introduce non-determinism.

### Options

| Option | Pros | Cons |
|--------|------|------|
| **A: Real-time clock** | Accurate timing for peripherals | Non-deterministic, host-dependent |
| **B: Step counter** (chosen) | Deterministic, reproducible, prevents infinite loops | No real-time guarantees |
| **C: Cycle-accurate timing** | Most accurate | Complex, slow, unnecessary for educational purposes |

### Decision

**Option B: Step counter (`max_steps`)**

### Rationale

1. **Determinism**: Same input always produces same output regardless of host load.
2. **Infinite loop protection**: Student code often ends with `B .` (infinite loop). Step limit prevents hang.
3. **Simplicity**: One integer parameter controls execution length.
4. **Educational appropriateness**: For learning ARM instructions, exact timing isn't critical.

### Consequences

- **Positive**: Reproducible results, simple to implement, safe
- **Negative**: Can't simulate real-time behavior (e.g., "blink LED every 500ms")
- **Mitigation**: Timer peripherals (TIM6) use step count as time base (1 step = 1 tick)

---

## ADR-004: Thumb-16 Only (No Thumb-32)

**Date:** March 2026  
**Status:** Accepted

### Problem

The ARM Thumb-2 instruction set includes both 16-bit and 32-bit instructions. Should the simulator support both?

### Options

| Option | Pros | Cons |
|--------|------|------|
| **A: Thumb-16 only** (chosen) | Simpler decoder, covers 90% of educational programs | BL/BLX not supported, limited function calls |
| **B: Thumb-16 + Thumb-32** | Complete instruction set | Complex decoding (check if second halfword needed), more testing |
| **C: ARM mode (32-bit only)** | Simpler decoding | Larger binaries, not used by Cortex-M3 (Thumb-only) |

### Decision

**Option A: Thumb-16 only**

### Rationale

1. **Coverage**: Most student programs use basic data processing, branches, and memory access — all available in Thumb-16.
2. **Complexity**: Thumb-32 requires checking bits [15:11] to determine if instruction is 16 or 32 bits, then decoding two halfwords together.
3. **Development speed**: Implementing all Thumb-16 instructions provides a solid foundation. Thumb-32 can be added incrementally.
4. **Educational focus**: Students learn core concepts (registers, memory, branching) before advanced features.

### Consequences

- **Positive**: Simpler codebase, faster implementation, covers most labs
- **Negative**: BL/BLX not available (no function calls), LDRD/STRD not available
- **Workaround**: Student programs should use inline code instead of function calls
- **Future**: Add BL (32-bit) as first Thumb-32 instruction (high educational value)

---

## ADR-005: stdout-Based Result Communication

**Date:** March 2026  
**Status:** Accepted

### Problem

How should the C simulator communicate execution results (register values, GPIO state, UART output) to the Go orchestrator?

### Options

| Option | Pros | Cons |
|--------|------|------|
| **A: stdout text parsing** (chosen) | Simple, no dependencies, human-readable | Fragile to format changes, regex-based |
| **B: JSON output to stdout** | Structured, easy to parse | Less human-readable, requires JSON library in C |
| **C: Output file** | Decoupled, persistent | File I/O overhead, cleanup required |
| **D: Shared memory / pipes** | Fast, structured | Complex IPC setup, platform-specific |

### Decision

**Option A: stdout text with `[TAG]` prefixes**

### Rationale

1. **Simplicity**: `printf()` is universally understood. No additional libraries needed in C.
2. **Human-readable**: Developers can read simulator output directly during debugging.
3. **Language-agnostic**: Any orchestrator (Go, Python, Java) can parse text output.
4. **Structured enough**: `[TAG]` prefixes (`[STATS]`, `[GPIO]`, `[UART]`) enable reliable regex extraction.

### Consequences

- **Positive**: Simple, debuggable, no dependencies
- **Negative**: Regex parsing is brittle; output format changes break orchestrator
- **Mitigation**: Document output format, add tests for parser, consider JSON output in future

---

## ADR-006: Dynamic Memory Allocation for Flash/SRAM

**Date:** March 2026  
**Status:** Accepted

### Problem

Should Flash and SRAM be statically allocated (arrays in struct) or dynamically allocated (calloc)?

### Options

| Option | Pros | Cons |
|--------|------|------|
| **A: Static arrays** | No allocation failure, no cleanup needed | Large stack/heap usage, inflexible |
| **B: Dynamic allocation** (chosen) | Controlled lifetime, flexible sizing | Allocation can fail, cleanup required |
| **C: Memory-mapped files** | Persistent, shareable | Complex, unnecessary for simulation |

### Decision

**Option B: Dynamic allocation via `calloc`**

### Rationale

1. **Size flexibility**: Can easily change Flash/SRAM sizes for different STM32 variants.
2. **Memory efficiency**: Allocated only when simulator runs, freed on exit.
3. **Zero initialization**: `calloc` zeroes memory (correct reset state).
4. **Reasonable sizes**: 64 KB + 20 KB = 84 KB total — small enough for dynamic allocation.

### Consequences

- **Positive**: Flexible, clean lifecycle (init → run → free)
- **Negative**: Must check for allocation failure, must call `memory_free()`
- **Risk**: Low — 84 KB allocation is trivial on modern systems

---

## ADR-007: Little-Endian Byte Ordering

**Date:** March 2026  
**Status:** Accepted (implicit — matches Cortex-M3 native)

### Problem

What byte ordering should the simulator use for multi-byte memory accesses?

### Decision

**Little-endian** (matching Cortex-M3 native ordering)

### Rationale

1. **Cortex-M3 native**: ARM Cortex-M3 uses little-endian by default.
2. **x86/x64 host**: Most development machines are little-endian, so no byte-swapping needed.
3. **Simplicity**: No endianness conversion code required.

### Consequences

- **Positive**: Native performance, matches hardware
- **Negative**: If running on big-endian host (rare), byte-swapping needed
- **Note**: Not an issue — big-endian ARM hosts are essentially extinct

---

## ADR-008: No Peripheral Bus Routing (Current Limitation)

**Date:** March 2026  
**Status:** Accepted (technical debt — must be fixed)

### Problem

Memory writes to peripheral addresses (`0x40000000+`) are silently dropped because `memory_write_byte` doesn't route them to GPIO/UART/TIM6 modules.

### Options

| Option | Pros | Cons |
|--------|------|------|
| **A: Add routing in `memory_write_byte`** | Centralized, works for all accesses | Requires `Simulator*` parameter in memory functions |
| **B: Separate peripheral dispatch in `simulator_step`** | No change to memory API | Duplicates address decoding |
| **C: Bus matrix module** | Realistic architecture | Adds complexity, another layer |

### Decision

**Option A: Add routing to `memory_write_byte`** (planned fix)

### Rationale

1. **Correctness**: All peripheral writes go through single point.
2. **Realism**: Matches actual memory-mapped I/O behavior.
3. **Minimal disruption**: Only `memory_write_byte` needs modification (plus read for peripheral registers).

### Consequences

- **Current**: GPIO/UART/TIM6 writes silently dropped (simulator works for pure register/CPU tests only)
- **After fix**: Full peripheral access, integration tests pass
- **Implementation effort**: Medium (2-4 hours) — requires refactoring memory API to accept `Simulator*`

---

#decisions #adr #architecture #trade-offs #rationale
