# 🧪 Руководство по тестированию STM32 Simulator

## 📋 Обзор тестов

Проект включает три уровня тестирования:

| Уровень | Файлы | Описание |
|---------|-------|----------|
| **Модульные** | `test_instructions.c`, `test_peripheral.c` | Тесты отдельных инструкций и периферии |
| **Расширенные** | `test_extended_instructions.c`, `test_peripheral_extended.c` | Детальные тесты компонентов |
| **Интеграционные** | `test_integration.c` | End-to-end тесты полного цикла |

---

## 🚀 Быстрый старт

### Linux/WSL

```bash
# Сборка
mkdir build && cd build
cmake ..
make

# Запуск всех тестов
cd ..
./run_tests.sh

# Или по отдельности
./build/test_instructions
./build/test_peripheral
./build/test_integration
```

### Windows (PowerShell)

```powershell
# Сборка
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make

# Запуск всех тестов
cd ..
.\run_tests.ps1

# Или по отдельности
.\build\test_instructions.exe
.\build\test_peripheral.exe
```

---

## 📁 Описание тестов

### 1. Модульные тесты инструкций

**Файл:** `test_instructions.c`

**Тестируемые инструкции:**
- ✅ `MOV Rd, #imm8` — загрузка константы
- ✅ `ADD Rd, Rn, Rm` — сложение
- ✅ `SUB Rd, Rn, Rm` — вычитание
- ✅ `B offset` — безусловный переход

**Пример запуска:**
```bash
./build/test_instructions
```

**Ожидаемый вывод:**
```
========================================
  Instruction Tests
========================================
Testing MOV instruction...
PASS: MOV instruction test
Testing ADD instruction...
PASS: ADD instruction test
...
All instruction tests PASSED!
```

---

### 2. Модульные тесты периферии

**Файл:** `test_peripheral.c`

**Тестируемые компоненты:**
- ✅ GPIO (порты A-G)
- ✅ TIM6 (таймер)
- ✅ NVIC (контроллер прерываний)
- ✅ Bus Matrix
- ✅ RCC (тактирование)
- ✅ DMA

**Пример запуска:**
```bash
./build/test_peripheral
```

---

### 3. Расширенные тесты инструкций

**Файл:** `test_extended_instructions.c`

**Тесты:**
1. **Arithmetic instructions** — ADD, SUB
2. **Logical instructions** — AND, EOR, ORR
3. **Branch instruction** — B (переход)
4. **Shift instructions** — LSL, LSR
5. **Load/store instructions** — LDR, STR
6. **Multi-instruction sequence** — последовательность инструкций
7. **CPU flags** — N, Z, C, V

**Пример запуска:**
```bash
./build/test_extended_instructions
```

---

### 4. Расширенные тесты периферии

**Файл:** `test_peripheral_extended.c`

**Тесты:**

#### TIM6:
- ✅ Initialization
- ✅ Register read/write (CR1, CNT, ARR)
- ✅ Control bits (CEN, UDIS)

#### USART:
- ✅ Initialization
- ✅ Register access (CR1, DR, SR)
- ✅ Transmission (TXE, TC flags)

#### NVIC:
- ✅ Initialization
- ✅ Enable/disable interrupts
- ✅ Pending interrupts

#### GPIO:
- ✅ Output operations (ODR, BSRR)

#### Memory:
- ✅ Flash/SRAM byte access
- ✅ Halfword/word access (little-endian)

**Пример запуска:**
```bash
./build/test_peripheral_extended
```

---

### 5. Интеграционные тесты

**Файл:** `test_integration.c`

**End-to-end тесты:**

1. **Demo program execution**
   - Загрузка демо-программы
   - Выполнение: MOV, ADD, B
   - Проверка регистров R0, R1, R2

2. **CPU + GPIO integration**
   - Запись в GPIO ODR
   - Чтение обратно

3. **USART transmission simulation**
   - Запись в DR
   - Проверка флагов TXE

4. **Memory map boundaries**
   - Проверка границ Flash (64KB)
   - Проверка границ SRAM (20KB)

5. **CPU reset state**
   - Проверка обнуления всех регистров

6. **Loop execution**
   - Тест зацикливания (B .)

7. **All GPIO ports**
   - Тест портов A-G

8. **Register file**
   - Доступность R0-R15

**Пример запуска:**
```bash
./build/test_integration
```

---

## 📊 Покрытие тестами

| Компонент | Покрытие | Тесты |
|-----------|----------|-------|
| **CPU (инструкции)** | 60% | MOV, ADD, SUB, B |
| **CPU (регистры)** | 100% | R0-R15, xPSR |
| **Память** | 100% | Flash, SRAM, byte/half/word |
| **GPIO** | 100% | Все регистры, порты A-G |
| **TIM6** | 80% | Регистры, control bits |
| **USART** | 80% | TX, флаги SR |
| **NVIC** | 70% | Enable/disable, pending |
| **Прерывания** | 0% | Не реализованы |

---

## 🔧 Добавление новых тестов

### Шаблон теста

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "cpu_state.h"
#include "execute.h"

int test_my_feature() {
    printf("Testing my feature...\n");
    
    Simulator sim;
    
    if (!memory_init(&sim.mem)) {
        printf("FAIL: Memory initialization\n");
        return 1;
    }
    
    gpio_init(&sim.gpio);
    cpu_reset(&sim.cpu);
    
    // Ваш тест здесь
    // ...
    
    if (/* условие провала */) {
        printf("FAIL: Description\n");
        memory_free(&sim.mem);
        return 1;
    }
    
    printf("PASS: Description\n");
    memory_free(&sim.mem);
    return 0;
}
```

### Регистрация в CMakeLists.txt

```cmake
add_executable(test_my_feature test/test_my_feature.c)
target_link_libraries(test_my_feature core_static)
```

### Добавление в скрипт запуска

**run_tests.sh:**
```bash
TESTS=(
    "test_gpio"
    "test_my_feature"  # Добавить сюда
)
```

---

## 📈 Запуск всех тестов

### Автоматический запуск

**Linux/WSL:**
```bash
./run_tests.sh
```

**Windows:**
```powershell
.\run_tests.ps1
```

### Вывод скрипта

```
========================================
  STM32 Simulator - Test Suite
========================================

Found 7 test executables

Running: test_gpio
----------------------------------------
Running GPIO tests...
All GPIO tests passed!
✓ PASSED

Running: test_simulator
----------------------------------------
...
✓ PASSED

========================================
  Test Summary
========================================

Total tests:  7
Passed:       7
Failed:       0

All tests passed!
```

---

## 🐛 Отладка тестов

### Вывод отладочной информации

Добавьте `printf` в тест:

```c
printf("DEBUG: R0 = 0x%08X\n", sim.cpu.regs[0]);
```

### Запуск с подробным выводом

```bash
./build/test_instructions 2>&1 | grep -E "(PASS|FAIL|DEBUG)"
```

### Проверка памяти

```c
// Перед тестом
printf("Memory before: flash[0] = 0x%02X\n", sim.mem.flash[0]);

// После теста
printf("Memory after: flash[0] = 0x%02X\n", sim.mem.flash[0]);
```

---

## ✅ Критерии прохождения тестов

### Для сдачи на "удовлетворительно":
- ✅ Все модульные тесты проходят
- ✅ `test_integration`: тесты 1-5

### Для сдачи на "хорошо":
- ✅ Всё из "удовлетворительно"
- ✅ `test_extended_instructions`: тесты 1-3
- ✅ `test_peripheral_extended`: TIM6, USART, NVIC

### Для сдачи на "отлично":
- ✅ Всё из "хорошо"
- ✅ 100% тестов проходят
- ✅ Добавлены новые тесты для прерываний

---

## 📚 Примеры тестовых программ

### Тест 1: Сложение чисел

```c
uint16_t program[] = {
    0x2005,  // MOV R0, #5
    0x2103,  // MOV R1, #3
    0x1882,  // ADD R2, R0, R1
    0xE7FE   // B .
};

// Ожидаемый результат: R2 = 8
```

### Тест 2: Мигание светодиодом (GPIO)

```c
// Запись в ODR
gpio_write_register(&gpio, GPIO_PORT_A_ADDR + GPIO_ODR_OFFSET, 0x0001);
// Чтение
uint32_t result = gpio_read_register(&gpio, GPIO_PORT_A_ADDR + GPIO_ODR_OFFSET);
// Ожидаемый результат: result = 0x0001
```

### Тест 3: Передача по UART

```c
// Запись в DR
usart_write_register(&usart, USART1_BASE_ADDR + USART_DR_OFFSET, 0x41);
// Проверка флага TXE
uint32_t sr = usart_read_register(&usart, USART1_BASE_ADDR + USART_SR_OFFSET);
// Ожидаемый результат: sr & USART_SR_TXE != 0
```

---

## 📞 Troubleshooting

### Ошибка: "Segmentation fault"

**Причина:** Доступ к неинициализированной памяти

**Решение:**
```c
if (!memory_init(&sim.mem)) {
    printf("FAIL: Memory init\n");
    return 1;
}
```

### Ошибка: "Test hangs"

**Причина:** Бесконечный цикл (B .)

**Решение:** Добавить счётчик шагов:
```c
int max_steps = 100;
int steps = 0;
while (steps < max_steps) {
    simulator_step(&sim);
    steps++;
}
```

### Ошибка: "Expected X, got Y"

**Причина:** Неправильный формат инструкции

**Решение:** Проверить encoding в PM0056

---

**Дата:** 13 марта 2026  
**Версия:** 1.0
