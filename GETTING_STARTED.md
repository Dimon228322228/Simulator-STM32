# 🚀 Быстрый старт

## Для Windows

### 1. Требования

Установите:
- **MSYS2** или **MinGW** (для GCC) — https://www.mingw-w64.org/
- **CMake** — https://cmake.org/download/
- **Python 3** (опционально) — https://www.python.org/
- **Go 1.21+** (опционально, для оркестратора) — https://go.dev/

### 2. Сборка симулятора

Откройте PowerShell в директории проекта:

```powershell
# Создаём директорию сборки
mkdir build
cd build

# Конфигурируем
cmake .. -G "MinGW Makefiles"

# Собираем
mingw32-make

# Проверяем
.\stm32_sim.exe --demo --max-steps 10
```

### 3. Создание тестовых программ

```powershell
# Создаём тестовые .bin файлы
python create_test_bin.py all

# Запускаем симулятор с тестовой программой
.\build\stm32_sim.exe demo.bin
```

### 4. Запуск тестов

```powershell
.\build\test_gpio.exe
.\build\test_simulator.exe
```

### 5. Сборка оркестратора (опционально)

```powershell
cd orchestrator

# Загружаем зависимости
go mod download

# Собираем
go build -o orchestrator.exe .

# Запускаем в режиме файла
.\orchestrator.exe --mode file --file ..\test_task.json --output result.json --simulator ..\build\stm32_sim.exe
```

---

## Для WSL / Linux

### 1. Требования

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential cmake git python3 python3-pip

# Для оркестратора (опционально)
sudo apt install -y golang-go
```

### 2. Сборка симулятора

```bash
cd C:\Code\embedded_programming\project\Simulator-STM32

# Создаём директорию сборки
mkdir build && cd build

# Конфигурируем и собираем
cmake ..
make

# Проверяем
./stm32_sim --demo --max-steps 10
```

### 3. Создание тестовых программ

```bash
# Создаём тестовые .bin файлы
python3 create_test_bin.py all

# Запускаем симулятор
./stm32_sim demo.bin
```

### 4. Запуск тестов

```bash
./test_gpio
./test_simulator
```

### 5. Сборка оркестратора (опционально)

```bash
cd orchestrator

# Загружаем зависимости
go mod download

# Собираем
go build -o orchestrator .

# Запускаем в режиме файла
./orchestrator --mode file --file ../test_task.json --output result.json --simulator ../build/stm32_sim
```

---

## Проверка работы

### Ожидаемый вывод симулятора

```
Initializing Simulator...
==========================

[INIT] Vector table initialized (MSP=0x20005000, PC=0x08000008)
[INIT] Loaded demo program (8 bytes)

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
[STATS] Memory reads: 0
[STATS] Memory writes: 0
========================================
```

**Что произошло:**
1. Загрузилась демо-программа (4 инструкции)
2. Выполнено: `MOV R0,#5`, `MOV R1,#3`, `ADD R2,R0,R1`, `B .`
3. Результат: `R0=5`, `R1=3`, `R2=8` (5+3)

---

## Решение проблем

### Ошибка: `cmake: not found`

```bash
# Linux
sudo apt install cmake

# Windows (Chocolatey)
choco install cmake
```

### Ошибка: `gcc: not found`

```bash
# Linux
sudo apt install build-essential

# Windows (MSYS2)
pacman -S mingw-w64-x86_64-gcc
```

### Ошибка: `go: command not found`

```bash
# Linux
sudo apt install golang-go

# Windows
# Скачайте с https://go.dev/dl/
```

### Ошибка при сборке: `undefined reference`

Убедитесь, что все файлы в директориях `core/src/` и `core/include/`:
```bash
ls core/src/
# Должны быть: cpu_state.c, execute.c, gpio.c, memory.c
```

### Оркестратор не подключается к Redis

```bash
# Запустите Redis (Docker)
docker run -d -p 6379:6379 redis:latest

# Или используйте режим файла
./orchestrator --mode file --file task.json --output result.json
```

---

## Следующие шаги

1. **Изучите код симулятора:**
   - `core/src/execute.c` — выполнение инструкций
   - `core/src/memory.c` — работа с памятью
   - `core/src/gpio.c` — GPIO периферия

2. **Добавьте новые инструкции:**
   - Откройте `core/src/execute.c`
   - Добавьте case в switch по образцу

3. **Напишите свою программу:**
   - Используйте `create_test_bin.py` как шаблон
   - Или создайте .bin файл в ассемблере

4. **Запустите оркестратор с Redis:**
   ```bash
   docker run -d -p 6379:6379 redis:latest
   ./orchestrator --mode queue --redis localhost:6379
   ```

5. **Прочитайте документацию:**
   - `README.md` — полная документация
   - `docs/` — документация по периферии

---

## Команды для быстрой проверки

```bash
# Linux/WSL
./quick_test.sh

# Windows (PowerShell)
.\quick_test.ps1
```
