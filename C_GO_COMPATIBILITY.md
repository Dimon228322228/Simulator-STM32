# Совместимость C и Go компонентов

## 📋 Обзор

Этот документ описывает формат взаимодействия между C-симулятором и Go-оркестратором.

---

## 🔄 Поток данных

```
┌─────────────────────┐
│  Go-оркестратор     │
│                     │
│  1. Создаёт .bin    │
│  2. Запускает C     │
│  3. Читает stdout   │
│  4. Парсит вывод    │
│  5. Возвращает      │
└──────────┬──────────┘
           │ exec.Command()
           ▼
┌─────────────────────┐
│  C-симулятор        │
│                     │
│  1. Читает .bin     │
│  2. Выполняет код   │
│  3. Выводит stdout  │
│  4. Выходит (0/1)   │
└─────────────────────┘
```

---

## 📤 Формат вывода C-симулятора

### 1. Статистика выполнения

**C-код (main.c):**
```c
printf("[STATS] Instructions executed: %u\n", stats.instructions_executed);
printf("[STATS] CPU cycles: %u\n", stats.cycles);
```

**Go-парсер (runner.go):**
```go
statsRegex := regexp.MustCompile(`\[STATS\] Instructions executed: (\d+)`)
cyclesRegex := regexp.MustCompile(`\[STATS\] CPU cycles: (\d+)`)
```

**Пример вывода:**
```
[STATS] Instructions executed: 4
[STATS] CPU cycles: 4
```

---

### 2. Состояние регистров CPU

**C-код (main.c):**
```c
printf("  R0:  0x%08X    R4:  0x%08X ...\n", cpu->regs[0], cpu->regs[4]);
printf("  R1:  0x%08X    R5:  0x%08X ...\n", cpu->regs[1], cpu->regs[5]);
printf("  R2:  0x%08X    R6:  0x%08X ...\n", cpu->regs[2], cpu->regs[6]);
printf("  R3:  0x%08X    R7:  0x%08X ...\n", cpu->regs[3], cpu->regs[7]);
```

**Go-парсер (runner.go):**
```go
regR0Regex := regexp.MustCompile(`R0:\s+0x([0-9A-Fa-f]+)`)
regR1Regex := regexp.MustCompile(`R1:\s+0x([0-9A-Fa-f]+)`)
regR2Regex := regexp.MustCompile(`R2:\s+0x([0-9A-Fa-f]+)`)
regR3Regex := regexp.MustCompile(`R3:\s+0x([0-9A-Fa-f]+)`)
```

**Пример вывода:**
```
[CPU] Final register state:
  R0:  0x00000005    R4:  0x00000000    R8:  0x00000000   R12: 0x%08X
  R1:  0x00000003    R5:  0x00000000    R9:  0x00000000   R13: 0x%08X (SP)
  R2:  0x00000008    R6:  0x00000000    R10: 0x%08X   R14: 0x%08X (LR)
  R3:  0x00000000    R7:  0x00000000    R11: 0x%08X   R15: 0x%08X (PC)
  xPSR: 0x%08X
```

**Результат парсинга:**
```json
{
  "gpio_state": {
    "R0": 5,
    "R1": 3,
    "R2": 8,
    "R3": 0
  }
}
```

---

### 3. UART вывод

**C-код (usart.c):**
```c
printf("[UART] TX: 0x%02X\n", value & 0xFF);
```

**Go-парсер (runner.go):**
```go
uartRegex := regexp.MustCompile(`\[UART\] TX: 0x([0-9A-Fa-f]{2})`)
```

**Пример вывода:**
```
[UART] TX: 0x48  // 'H'
[UART] TX: 0x65  // 'e'
[UART] TX: 0x6C  // 'l'
[UART] TX: 0x6C  // 'l'
[UART] TX: 0x6F  // 'o'
```

**Результат парсинга:**
```json
{
  "uart_output": "Hello"
}
```

---

### 4. GPIO состояние

**C-код (gpio.c):**
```c
printf("[GPIO] PORT_%c_ODR = 0x%08X\n", 'A' + port_idx, value);
```

**Go-парсер (runner.go):**
```go
gpioRegex := regexp.MustCompile(`\[GPIO\] PORT_(\w)_ODR = 0x([0-9A-Fa-f]+)`)
// В коде:
portName := matches[1]  // A, B, C...
result.GPIOState[fmt.Sprintf("PORT_%s_ODR", portName)] = uint32(val)
```

**Пример вывода:**
```
[GPIO] PORT_A_ODR = 0x00000005
[GPIO] PORT_B_ODR = 0x000000FF
```

**Результат парсинга:**
```json
{
  "gpio_state": {
    "PORT_A_ODR": 5,
    "PORT_B_ODR": 255
  }
}
```

---

### 5. Сообщения об ошибках

**C-код (main.c):**
```c
fprintf(stderr, "[ERROR] Cannot open file: %s\n", filename);
```

**Go-парсер (runner.go):**
```go
if strings.Contains(output, "[ERROR]") {
    result.Status = "error"
    errorRegex := regexp.MustCompile(`\[ERROR\] (.+)`)
    // ...
}
```

**Пример вывода:**
```
[ERROR] Cannot open file: nonexistent.bin
```

**Результат парсинга:**
```json
{
  "status": "error",
  "error_message": "Cannot open file: nonexistent.bin"
}
```

---

## 📊 Сводная таблица форматов

| Компонент | Формат C | Regex Go | Результат |
|-----------|----------|----------|-----------|
| **Инструкции** | `[STATS] Instructions executed: %u` | `\[STATS\] Instructions executed: (\d+)` | `int` |
| **Циклы** | `[STATS] CPU cycles: %u` | `\[STATS\] CPU cycles: (\d+)` | `int` |
| **Регистры** | `R0:  0x%08X` | `R0:\s+0x([0-9A-Fa-f]+)` | `uint32` |
| **UART TX** | `[UART] TX: 0x%02X` | `\[UART\] TX: 0x([0-9A-Fa-f]{2})` | `string` |
| **GPIO ODR** | `[GPIO] PORT_X_ODR = 0x%08X` | `\[GPIO\] PORT_(\w)_ODR = 0x(...)` | `map[string]uint32` |
| **Ошибки** | `[ERROR] ...` | `\[ERROR\] (.+)` | `string` |

---

## 🔧 Добавление нового формата вывода

### Шаг 1: C-симулятор

Добавьте `printf` в соответствующий файл периферии:

```c
// Пример для TIM6
printf("[TIM6] CNT = %u\n", tim6->cnt);
```

### Шаг 2: Go-оркестратор

Добавьте регулярное выражение и парсинг:

```go
// В runner.go
tim6Regex := regexp.MustCompile(`\[TIM6\] CNT = (\d+)`)

// В цикле парсинга:
if matches := tim6Regex.FindStringSubmatch(line); matches != nil {
    if val, err := strconv.Atoi(matches[1]); err == nil {
        result.GPIOState["TIM6_CNT"] = uint32(val)
    }
}
```

### Шаг 3: Тестирование

Проверьте полный цикл:

```bash
# C-симулятор
./stm32_sim test.bin

# Go-оркестратор
./orchestrator --mode file --file task.json --output result.json

# Проверьте result.json
cat result.json | jq .
```

---

## ⚠️ Важные замечания

1. **Формат чисел:**
   - HEX: `0x%08X` (8 символов, верхний регистр)
   - DEC: `%u` (без знака)
   - BYTE: `0x%02X` (2 символа)

2. **Теги формата:**
   - Все теги в квадратных скобках: `[TAG]`
   - Теги должны быть уникальными

3. **Совместимость:**
   - При изменении формата вывода обновляйте Go-парсер
   - Тестируйте полный цикл после изменений

4. **Производительность:**
   - Вывод в stdout замедляет симуляцию
   - Для отладки используйте `-DNDEBUG` или флаги

---

## 📝 Пример полного вывода

```
Initializing Simulator...
==========================

[INIT] Loaded 8 bytes from demo.bin into Flash at 0x08000000

Starting simulation...
----------------------


==========================
Simulation finished.

[CPU] Final register state:
  R0:  0x00000005    R4:  0x00000000    R8:  0x00000000   R12: 0x00000000
  R1:  0x00000003    R5:  0x00000000    R9:  0x00000000   R13: 0x00000000 (SP)
  R2:  0x00000008    R6:  0x00000000    R10: 0x00000000   R14: 0x00000000 (LR)
  R3:  0x00000000    R7:  0x00000000    R11: 0x00000000   R15: 0x0800000E (PC)
  xPSR: 0x00000000

========================================
       SIMULATION STATISTICS
========================================
[STATS] Instructions executed: 4
[STATS] CPU cycles: 4
[STATS] UART bytes sent: 0
========================================
```

**Результат парсинга:**
```json
{
  "task_id": "test-001",
  "status": "success",
  "instructions_executed": 4,
  "cycles": 4,
  "gpio_state": {
    "R0": 5,
    "R1": 3,
    "R2": 8
  },
  "uart_output": ""
}
```

---

**Дата:** 13 марта 2026  
**Версия:** 1.0
