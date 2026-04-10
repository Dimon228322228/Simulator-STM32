# 📝 Шпаргалка по проекту STM32 Simulator

## 🎯 Что это

Симулятор микроконтроллера STM32F103C8T6 (Cortex-M3) + Go-оркестратор для облачной лаборатории.

---

## ⚡ Быстрый старт (5 минут)

### Windows (PowerShell)

```powershell
# 1. Сборка
mkdir build; cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
cd ..

# 2. Тест
.\build\stm32_sim.exe --demo --max-steps 10

# 3. Создать тестовую программу
python create_test_bin.py demo

# 4. Запустить
.\build\stm32_sim.exe demo.bin
```

### Linux / WSL

```bash
# 1. Сборка
mkdir build && cd build
cmake .. && make
cd ..

# 2. Тест
./stm32_sim --demo --max-steps 10

# 3. Создать тестовую программу
python3 create_test_bin.py demo

# 4. Запустить
./stm32_sim demo.bin
```

---

## 📁 Структура проекта

```
Simulator-STM32/
├── core/           # Симулятор (C)
│   ├── main.c      # Точка входа
│   ├── src/        # Исходники
│   └── include/    # Заголовки
├── orchestrator/   # Оркестратор (Go)
│   ├── main.go
│   ├── keydb/
│   └── simulator/
├── test/           # Тесты
├── build/          # Сборка
├── README.md       # Документация
└── GETTING_STARTED.md  # Подробный старт
```

---

## 🔧 Основные команды

### Симулятор (C)

```bash
# Сборка
cd build && cmake .. && make

# Запуск
./stm32_sim --help              # Справка
./stm32_sim --demo              # Демо-режим
./stm32_sim program.bin         # Загрузить .bin файл
./stm32_sim -v --max-steps 100  # Подробно, макс. 100 инструкций

# Тесты
./test_gpio
./test_simulator
```

### Оркестратор (Go)

```bash
# Сборка
cd orchestrator
go mod download
go build -o orchestrator .

# Запуск
./orchestrator --mode file --file task.json --output result.json
./orchestrator --mode queue --redis localhost:6379
```

### Тестовые бинарники

```bash
# Создать
python3 create_test_bin.py demo      # demo.bin
python3 create_test_bin.py loop      # loop.bin
python3 create_test_bin.py memory    # memory.bin
python3 create_test_bin.py all       # Все файлы

# Запустить
./stm32_sim demo.bin --max-steps 20
```

---

## 📚 Формат задачи (JSON)

```json
{
  "task_id": "lab1-001",
  "student_id": "student_42",
  "lab_number": 1,
  "binary": "BQUDGII=",
  "timeout_sec": 5,
  "config": {}
}
```

**binary** — это .bin файл в base64.

---

## 🐳 Docker

```bash
# Сборка образа
docker build -t stm32-simulator:latest .

# Запуск симулятора
docker run --rm stm32-simulator /app/stm32_sim --demo

# Запуск оркестратора
docker run --rm -v $(pwd):/data stm32-simulator \
  /app/orchestrator --mode file --file /data/task.json
```

---

## 🧪 Примеры программ

### Демо (сложение)

```python
# demo.py
instructions = [
    0x2005,  # MOV R0, #5
    0x2103,  # MOV R1, #3
    0x1882,  # ADD R2, R0, R1
    0xE7FE,  # B .
]
```

**Результат:** R0=5, R1=3, R2=8

### Цикл

```python
# loop.py
instructions = [
    0x2000,  # MOV R0, #0
    0x210A,  # MOV R1, #10
    0x1C40,  # ADDS R0, R0, #1
    0x4288,  # CMP R0, R1
    0xDBFB,  # BLT Loop
    0xE7FE,  # B .
]
```

**Результат:** R0=10 (после 10 итераций)

---

## 🎓 Реализованные инструкции

| Инструкция | Описание | Код |
|------------|----------|-----|
| `MOV Rd, #imm8` | Загрузка константы | `00100 Rd imm8` |
| `ADD Rd, Rn, Rm` | Сложение | `0001100 Rm Rn Rd` |
| `SUB Rd, Rn, Rm` | Вычитание | `0001101 Rm Rn Rd` |
| `B offset` | Переход | `11100 offset` |

---

## 🔍 Отладка

### Вывод симулятора

```
[INIT] Loaded 8 bytes from demo.bin
[CPU] Final register state:
  R0: 0x00000005  R1: 0x00000003  R2: 0x00000008
[STATS] Instructions executed: 4
```

### Частые ошибки

| Ошибка | Решение |
|--------|---------|
| `cmake: not found` | `apt install cmake` или скачать с cmake.org |
| `gcc: not found` | `apt install build-essential` или MinGW |
| `go: not found` | `apt install golang-go` |
| `redis connection refused` | Запустить Redis: `docker run -d -p 6379:6379 redis` |

---

## 📖 Документация

| Файл | Описание |
|------|----------|
| `README.md` | Полная документация |
| `GETTING_STARTED.md` | Подробный старт |
| `IMPLEMENTATION_SUMMARY.md` | Что реализовано |
| `CHEATSHEET.md` | Этот файл |

---

## 🔗 Полезные ссылки

- [PM0056](https://www.st.com/resource/en/programming_manual/pm0056-cortexm3-programming-manual-stmicroelectronics.pdf) — инструкции Cortex-M3
- [STM32F103 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf) — документация
- [Thumb Instruction Set](https://developer.arm.com/documentation/ddi0301/h/) — система команд

---

## 💡 Советы

1. **Начни с демо:** `./stm32_sim --demo`
2. **Создай тест:** `python3 create_test_bin.py demo`
3. **Запусти:** `./stm32_sim demo.bin`
4. **Изучи код:** `core/src/execute.c` — выполнение инструкций
5. **Добавь инструкцию:** По образцу в `execute.c`

---

**Версия:** 1.0  
**Дата:** 13 марта 2026
