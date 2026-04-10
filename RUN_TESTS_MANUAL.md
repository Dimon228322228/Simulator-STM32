# 🧪 Руководство по запуску тестов STM32 Simulator

**Последнее обновление:** 13 марта 2026  
**Версия:** 1.0

---

## 📋 Содержание

1. [Быстрый старт](#быстрый-старт)
2. [Автоматический запуск](#автоматический-запуск)
3. [Ручной запуск](#ручной-запуск)
4. [Описание тестов](#описание-тестов)
5. [Интерпретация результатов](#интерпретация-результатов)
6. [Troubleshooting](#troubleshooting)

---

## 🚀 Быстрый старт

### Linux/WSL

```bash
# Перейти в директорию проекта
cd ~/esp_project/Simulator-STM32

# Запустить все тесты
./run_all_tests.sh

# Запустить конкретный тест
./run_all_tests.sh test_gpio
```

### Windows (PowerShell)

```powershell
# Перейти в директорию проекта
cd C:\Code\embedded_programming\project\Simulator-STM32

# Разрешить выполнение скриптов (если нужно)
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# Запустить все тесты
.\run_all_tests.ps1

# Запустить конкретный тест
.\run_all_tests.ps1 test_gpio
```

---

## 🤖 Автоматический запуск

### Скрипт `run_all_tests.sh` (Linux/WSL)

**Базовое использование:**

```bash
# Запустить все тесты
./run_all_tests.sh

# Подробный вывод
./run_all_tests.sh --verbose

# Только итоги (быстрый режим)
./run_all_tests.sh --quick

# Запустить один тест
./run_all_tests.sh test_gpio

# Комбинация опций
./run_all_tests.sh -q test_simulator
```

**Опции:**

| Опция | Короткая | Описание |
|-------|----------|----------|
| `--verbose` | `-v` | Подробный вывод каждого теста |
| `--quick` | `-q` | Только итоги (без деталей) |
| `--help` | `-h` | Показать справку |

**Пример вывода:**

```
============================================================================
  STM32 Simulator - Test Suite
============================================================================

────────────────────────────────────────────────────────────────
Running: test_gpio
────────────────────────────────────────────────────────────────
Running GPIO tests...

Testing GPIO initialization...
PASS: GPIO initialization test
...

✓ PASSED

...

============================================================================
  Test Summary
============================================================================

  Total tests:  7
  Passed:       5
  Failed:       2
  Skipped:      0

============================================================================

Success rate: 71%

❌ Some tests failed!
```

---

### Скрипт `run_all_tests.ps1` (Windows PowerShell)

**Базовое использование:**

```powershell
# Запустить все тесты
.\run_all_tests.ps1

# Подробный вывод
.\run_all_tests.ps1 -Verbose

# Только итоги (быстрый режим)
.\run_all_tests.ps1 -Quick

# Запустить один тест
.\run_all_tests.ps1 test_gpio

# Показать справку
.\run_all_tests.ps1 -Help
```

**Пример вывода:**

```
============================================================================
  STM32 Simulator - Test Suite
============================================================================

────────────────────────────────────────────────────────────────
Running: test_gpio
────────────────────────────────────────────────────────────────
Running GPIO tests...
...
✓ PASSED

...

============================================================================
  Test Summary
============================================================================

  Total tests:  7
  Passed:       5
  Failed:       2
  Skipped:      0

============================================================================

Success rate: 71%

❌ Some tests failed!
```

---

## 🙌 Ручной запуск

### По отдельности

```bash
cd ~/esp_project/Simulator-STM32/build

# Запустить каждый тест вручную
./test_gpio
./test_simulator
./test_instructions
./test_peripheral
./test_extended_instructions
./test_peripheral_extended
./test_integration
```

### С фильтром вывода

```bash
# Показать только PASS/FAIL
./test_gpio 2>&1 | grep -E "(PASS|FAIL)"

# Показать только тесты
./test_gpio 2>&1 | grep "Testing"

# Сохранить вывод в файл
./test_integration > test_results.txt 2>&1

# Показать последние 20 строк
./test_integration 2>&1 | tail -20
```

### Все тесты одной командой

```bash
cd ~/esp_project/Simulator-STM32/build

# Запустить все тесты последовательно
for test in test_*; do echo "=== $test ==="; ./$test || true; done

# Или с разделителями
for test in test_gpio test_simulator test_instructions test_peripheral \
            test_extended_instructions test_peripheral_extended test_integration; do
    echo "========================================"
    echo "Running: $test"
    echo "========================================"
    ./$test || echo "FAILED: $test"
    echo ""
done
```

---

## 📁 Описание тестов

### 1. test_gpio (Модульные тесты GPIO)

**Что тестирует:**
- Инициализация GPIO портов
- Сброс состояния
- Чтение/запись регистров (CRL, CRH, IDR, ODR, BSRR, BRR)
- Установка/сброс битов через BSRR

**Время выполнения:** ~0.1 сек  
**Сложность:** ⭐

**Команда:**
```bash
./test_gpio
```

---

### 2. test_simulator (Базовые тесты симулятора)

**Что тестирует:**
- Инициализация памяти
- Сброс CPU
- Выполнение инструкции MOV
- Доступ к Flash/SRAM

**Время выполнения:** ~0.1 сек  
**Сложность:** ⭐

**Команда:**
```bash
./test_simulator
```

---

### 3. test_instructions (Тесты системы команд)

**Что тестирует:**
- MOV (загрузка константы)
- ADD (сложение)
- SUB (вычитание)
- AND (логическое И)
- B (безусловный переход)
- LDR (загрузка из памяти)

**Время выполнения:** ~0.2 сек  
**Сложность:** ⭐⭐

**Команда:**
```bash
./test_instructions
```

---

### 4. test_peripheral (Тесты периферии)

**Что тестирует:**
- GPIO (порты A-G)
- TIM6 (таймер)
- Bus Matrix
- RCC (тактирование)
- DMA

**Время выполнения:** ~0.2 сек  
**Сложность:** ⭐⭐

**Команда:**
```bash
./test_peripheral
```

---

### 5. test_extended_instructions (Расширенные тесты инструкций)

**Что тестирует:**
- Арифметика (ADD, SUB)
- Логика (AND, EOR, ORR)
- Ветвления (B)
- Сдвиги (LSL, LSR)
- Память (LDR, STR)
- Последовательности инструкций
- Флаги CPU (N, Z, C, V)

**Время выполнения:** ~0.3 сек  
**Сложность:** ⭐⭐⭐

**Команда:**
```bash
./test_extended_instructions
```

---

### 6. test_peripheral_extended (Расширенные тесты периферии)

**Что тестирует:**
- TIM6 (инициализация, регистры, биты управления)
- USART (инициализация, передача, флаги)
- NVIC (прерывания, pending, enabled)
- GPIO (ODR, BSRR)
- Память (byte/halfword/word доступы)

**Время выполнения:** ~0.3 сек  
**Сложность:** ⭐⭐⭐

**Команда:**
```bash
./test_peripheral_extended
```

---

### 7. test_integration (Интеграционные тесты)

**Что тестирует:**
1. Demo program execution (MOV, ADD, B)
2. CPU + GPIO интеграция
3. USART transmission
4. Memory map boundaries
5. CPU reset state
6. Loop execution
7. Все GPIO порты (A-G)
8. Register file (R0-R15)

**Время выполнения:** ~0.5 сек  
**Сложность:** ⭐⭐⭐⭐

**Команда:**
```bash
./test_integration
```

---

## 📊 Интерпретация результатов

### Успешный тест

```
Testing GPIO initialization...
PASS: GPIO initialization test
```

✅ **Значение:** Тест прошёл, функциональность работает корректно.

---

### Упавший тест

```
FAIL: ADD instruction test failed. Expected R0=25, got R0=0
```

❌ **Значение:** Тест не прошёл. Ожидаемое значение не совпало с фактическим.

**Возможные причины:**
- Инструкция не реализована
- Неправильная декодировка инструкции
- Ошибка в тесте
- Проблема с инициализацией

---

### Пропущенный тест

```
⊘ SKIPPED: test_xyz (not found)
```

⚠️ **Значение:** Тест не найден в build директории.

**Причины:**
- Тест не скомпилирован
- Неправильное имя теста
- Проблема со сборкой

---

### Процент успеха

| Процент | Статус | Рекомендация |
|---------|--------|--------------|
| 100% | ✅ Отлично | Все тесты прошли |
| 80-99% | ⚠️ Хорошо | Есть мелкие проблемы |
| 50-79% | ⚠️ Удовл. | Требуются исправления |
| <50% | ❌ Плохо | Критичные проблемы |

---

## 🐛 Troubleshooting

### Ошибка: "command not found"

**Проблема:** Скрипт не найден или нет прав на выполнение.

**Решение:**
```bash
# Проверить наличие файла
ls -la run_all_tests.sh

# Дать права на выполнение
chmod +x run_all_tests.sh

# Запустить с указанием интерпретатора
bash run_all_tests.sh
```

---

### Ошибка: "Build directory not found"

**Проблема:** Директория build не существует или пуста.

**Решение:**
```bash
# Перейти в проект
cd ~/esp_project/Simulator-STM32

# Пересобрать
rm -rf build
mkdir build && cd build
cmake ..
make
```

---

### Ошибка: "Tests not found"

**Проблема:** Тесты не скомпилированы.

**Решение:**
```bash
cd ~/esp_project/Simulator-STM32/build

# Проверить наличие тестов
ls -la test_*

# Если нет - пересобрать
make clean
cmake ..
make
```

---

### Тесты падают с "Segmentation fault"

**Проблема:** Ошибка доступа к памяти.

**Возможные причины:**
- Неинициализированная память
- Выход за границы массива
- Null pointer dereference

**Решение:**
1. Запустить тест с отладкой:
   ```bash
   gdb ./test_gpio
   (gdb) run
   ```

2. Проверить вывод на конкретную ошибку

3. Исправить код или сообщить разработчику

---

### Тесты "зависают"

**Проблема:** Бесконечный цикл в тесте.

**Решение:**
```bash
# Запустить с таймаутом
timeout 5 ./test_integration

# Или в фоне с возможностью убить
./test_integration &
# Нажать Ctrl+C если завис
```

---

## 📈 CI/CD Интеграция

### GitHub Actions

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake gcc
    
    - name: Build
      run: |
        mkdir build && cd build
        cmake ..
        make
    
    - name: Run tests
      run: |
        cd build
        ./run_all_tests.sh --verbose
```

### GitLab CI

```yaml
test:
  stage: test
  image: ubuntu:22.04
  
  before_script:
    - apt-get update && apt-get install -y cmake gcc
  
  script:
    - mkdir build && cd build
    - cmake ..
    - make
    - ./run_all_tests.sh --verbose
  
  artifacts:
    when: always
    reports:
      junit: build/test_results.xml
```

---

## 📞 Контакты

По вопросам тестирования обращайтесь:
- Email: [your-email@domain.com]
- GitHub Issues: [ссылка на issues]
- Документация: [ссылка на docs]

---

**Приятного тестирования! 🎉**
