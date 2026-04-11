# STM32 Simulator — Knowledge Base

> [!info] Project Overview
> **STM32F103C8T6 Simulator** — a software emulator of the Cortex-M3 microcontroller, designed for the "Embedded Systems Programming" course at ITMO University (Spring 2026).

## 🎯 Goals

- Execute compiled `.bin` firmware files for STM32F103C8T6 on a host machine (Linux/WSL/Windows) without physical hardware
- Model the CPU core, memory subsystem, and peripheral blocks (GPIO, TIM6, UART, NVIC, SPI, I2C, DMA, RCC)
- Provide a microservice orchestrator (Go) for integration with the ITMO.clab cloud laboratory via KeyDB/Redis queues
- Serve as an educational tool for understanding Cortex-M3 architecture, instruction set, and embedded system principles

## 📦 System Composition

| Component | Language | Purpose |
|-----------|----------|---------|
| [[cpu_core\|CPU Core]] | C | Fetch-decode-execute cycle for Thumb-16 instructions |
| [[memory_model\|Memory Model]] | C | Flash (64 KB) and SRAM (20 KB) emulation |
| [[gpio\|GPIO]] | C | General-purpose I/O ports A–G |
| [[timer\|TIM6 Timer]] | C | General-purpose timer with interrupt generation |
| [[uart\|UART/USART]] | C | Serial communication (USART1, USART2, USART3) |
| [[interrupt_controller\|NVIC]] | C | Nested Vectored Interrupt Controller |
| [[instruction_execution\|Instruction Decoder]] | C | Thumb-16 instruction decoding and execution |
| [[microservice\|Orchestrator]] | Go | Task queue management, simulator lifecycle |
| [[testing_strategy\|Test Suite]] | C + Go + Python | Unit, integration, and system tests |

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    ITMO.clab (Web UI)                    │
└────────────────────────┬────────────────────────────────┘
                         │ JSON task (base64 .bin)
                         ▼
┌─────────────────────────────────────────────────────────┐
│                   KeyDB / Redis Queue                     │
│            simulator:tasks → simulator:results            │
└────────────────────────┬────────────────────────────────┘
                         │ BLPOP / RPUSH
                         ▼
┌─────────────────────────────────────────────────────────┐
│              Go Orchestrator (microservice)               │
│   ┌──────────┐  ┌───────────────┐  ┌──────────────────┐ │
│   │ keydb/   │  │ simulator/    │  │ types/           │ │
│   │ client.go│  │ runner.go     │  │ types.go         │ │
│   └──────────┘  └───────────────┘  └──────────────────┘ │
└────────────────────────┬────────────────────────────────┘
                         │ exec.Command()
                         ▼
┌─────────────────────────────────────────────────────────┐
│                C Simulator (core)                         │
│  ┌──────┐ ┌───────┐ ┌─────┐ ┌────┐ ┌────┐ ┌────┐       │
│  │ CPU  │ │Memory │ │GPIO │ │TIM6│ │NVIC│ │UART│  ...  │
│  └──────┘ └───────┘ └─────┘ └────┘ └────┘ └────┘       │
│  execute.c  memory.c  gpio.c  tim6.c  nvic.c  usart.c   │
└─────────────────────────────────────────────────────────┘
```

## 📂 Documentation Structure

### [[architecture/overall_architecture\|Architecture]]
- [[architecture/overall_architecture\|Overall Architecture]]
- [[architecture/cpu_core\|CPU Core]]
- [[architecture/memory_model\|Memory Model]]
- [[architecture/interrupt_controller\|Interrupt Controller (NVIC)]]
- [[architecture/timer\|TIM6 Timer]]
- [[architecture/uart\|UART/USART]]
- [[architecture/gpio\|GPIO Ports]]
- [[architecture/microservice\|Go Microservice]]
- [[architecture/data_flow\|Data Flow]]

### [[implementation/instruction_execution\|Implementation]]
- [[implementation/instruction_execution\|Instruction Execution]]
- [[implementation/instruction_decoder\|Instruction Decoder]]
- [[implementation/registers\|CPU Registers]]
- [[implementation/memory_access\|Memory Access]]
- [[implementation/peripherals_implementation\|Peripherals Implementation]]
- [[implementation/microservice_logic\|Microservice Logic]]

### [[testing/testing_strategy\|Testing]]
- [[testing/testing_strategy\|Testing Strategy]]
- [[testing/unit_tests\|Unit Tests]]
- [[testing/integration_tests\|Integration Tests]]
- [[testing/system_tests\|System Tests]]
- [[testing/test_infrastructure\|Test Infrastructure]]

### [[requirements/functional_requirements\|Requirements]]
- [[requirements/functional_requirements\|Functional Requirements]]
- [[requirements/non_functional_requirements\|Non-Functional Requirements]]
- [[requirements/system_features\|System Features]]

### [[tech/stm32_architecture\|Technology Stack]]
- [[tech/stm32_architecture\|STM32 Architecture]]
- [[tech/arm_architecture\|ARM Cortex-M3]]
- [[tech/go_microservice\|Go Microservice]]
- [[tech/keydb\|KeyDB/Redis]]
- [[tech/opentelemetry\|OpenTelemetry]]
- [[tech/testing_tools\|Testing Tools]]

### [[decisions/|Architectural Decisions]]
- Decision records (problem → options → chosen solution → rationale)

### [[glossary\|Glossary]]
- Terms, abbreviations, and definitions

---

#architecture #overview #stm32 #cortex-m3 #embedded
