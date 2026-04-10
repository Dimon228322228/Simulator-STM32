# 📋 План дальнейших шагов

**На основе аудита проекта от 13 марта 2026**

---

## 🎯 Цель

Довести проект до **100% соответствия ТЗ** для успешной сдачи курса.

---

## 🔴 Приоритет 1: Критичные задачи (блокируют сдачу)

### 1.1. Реализация прерываний (Cortex-M3 Exception Model)

**Файлы:** `core/src/execute.c`, `core/src/nvic.c`, `core/include/cpu_state.h`

**Что нужно сделать:**

1. **Добавить режим выполнения в обработчик прерываний:**
   ```c
   // В execute.c, перед simulator_step()
   if (nvic_has_pending_interrupt(&sim->nvic)) {
       enter_exception_handler(&sim->cpu, &sim->nvic, irq_number);
   }
   ```

2. **Реализовать stacking контекста:**
   ```c
   void enter_exception_handler(CPU_State *cpu, NVIC_State *nvic, uint8_t irq) {
       // Сохраняем R0-R3, R12, LR, PC, xPSR в стек
       cpu->msp -= 32;  // 8 регистров × 4 байта
       memory_write_word(..., cpu->regs[0]);
       // ... остальные регистры
       
       // Загружаем вектор прерывания
       cpu->pc = read_vector_table(irq);
       
       // Устанавливаем LR в EXC_RETURN (0xFFFFFFF9)
       cpu->regs[14] = 0xFFFFFFF9;
   }
   ```

3. **Реализовать unstacking при возврате:**
   ```c
   if (cpu->pc == 0xFFFFFFF9 || cpu->pc == 0xFFFFFFE9) {
       // Восстанавливаем регистры из стека
       cpu->regs[0] = memory_read_word(..., cpu->msp);
       // ...
       cpu->msp += 32;
   }
   ```

4. **Связать NVIC с execute.c:**
   ```c
   // В конце simulator_step()
   if (sim->nvic.pending && sim->nvic.enabled) {
       // Вход в прерывание
   }
   ```

**Время:** 8-12 часов  
**Сложность:** ⭐⭐⭐⭐⭐

---

### 1.2. Завершение TIM6 (таймер + прерывания)

**Файлы:** `core/src/tim6.c`, `core/include/tim6.h`

**Что нужно сделать:**

1. **Реализовать счёт тактов:**
   ```c
   void tim6_step(TIM6_State *tim6, uint32_t cpu_cycles) {
       if (tim6->regs.cr1 & TIM6_CR1_CEN) {
           // Увеличиваем счётчик
           tim6->regs.cnt++;
           
           // Проверка на переполнение
           if (tim6->regs.cnt >= tim6->regs.arr) {
               tim6->regs.cnt = 0;
               tim6->regs.sr |= TIM6_SR_UIF;  // Флаг прерывания
               
               // Запрос прерывания в NVIC
               if (tim6->regs.dier & TIM6_DIER_UIE) {
                   nvic_set_pending(nvic, TIM6_IRQn);
               }
           }
       }
   }
   ```

2. **Вызывать в execute.c:**
   ```c
   void simulator_step(Simulator *sim) {
       // ... выполнение инструкции
       stats.cycles++;
       
       // Обновляем таймеры
       tim6_step(&sim->tim6, stats.cycles);
   }
   ```

**Время:** 4-6 часов  
**Сложность:** ⭐⭐⭐

---

### 1.3. Приём UART (RX)

**Файлы:** `core/src/usart.c`, `core/include/usart.h`

**Что нужно сделать:**

1. **Добавить буфер приёма:**
   ```c
   typedef struct {
       // ... существующие поля
       uint8_t rx_buffer[256];
       uint16_t rx_head;
       uint16_t rx_tail;
       uint16_t rx_count;
   } USART_State;
   ```

2. **Функция внешнего ввода:**
   ```c
   void usart_receive(USART_State *usart, uint8_t data) {
       if (usart->rx_count < 256) {
           usart->rx_buffer[usart->rx_head] = data;
           usart->rx_head = (usart->rx_head + 1) % 256;
           usart->rx_count++;
           
           // Устанавливаем флаг RXNE
           usart->regs.sr |= USART_SR_RXNE;
           
           // Прерывание
           if (usart->regs.cr1 & USART_CR1_RXNEIE) {
               nvic_set_pending(nvic, USART1_IRQn);
           }
       }
   }
   ```

3. **Чтение из DR:**
   ```c
   case USART_DR_OFFSET:
       // При чтении DR возвращаем данные из RX буфера
       if (usart->rx_count > 0) {
           usart->regs.dr = usart->rx_buffer[usart->rx_tail];
           usart->rx_tail = (usart->rx_tail + 1) % 256;
           usart->rx_count--;
           
           // Сбрасываем RXNE если буфер пуст
           if (usart->rx_count == 0) {
               usart->regs.sr &= ~USART_SR_RXNE;
           }
       }
       break;
   ```

**Время:** 3-4 часа  
**Сложность:** ⭐⭐⭐

---

### 1.4. PDF отчёт

**Требования ТЗ (раздел 9):**

- Титульный лист (название, ФИО, руководитель, организация, год)
- Оглавление
- Введение (актуальность, цели, задачи)
- Техническое задание
- Описание архитектуры
- Описание реализации (структуры, алгоритмы, интерфейсы)
- Система тестирования
- Заключение (результаты, выводы)
- Список литературы (ГОСТ + официальная документация)

**Что делать:**

1. Использовать `IMPLEMENTATION_SUMMARY.md` как основу
2. Добавить титульный лист по стандарту ИТМО
3. Оформить по ГОСТ (шрифты, поля, нумерация)
4. Экспортировать в PDF

**Время:** 6-8 часов  
**Сложность:** ⭐⭐

---

## 🟡 Приоритет 2: Желательные задачи

### 2.1. Детализация счётчика тактов

**Файл:** `core/src/execute.c`

**Что нужно сделать:**

1. **Добавить таблицу тактов:**
   ```c
   typedef struct {
       uint8_t opcode;
       uint8_t cycles;
   } InstructionTiming;
   
   // Согласно PM0056
   static const InstructionTiming timing_table[] = {
       {0b00100, 1},  // MOV
       {0b00011, 1},  // ADD/SUB
       {0b11100, 2},  // B (branch)
       {0b11000, 2},  // LDR (memory access)
       // ...
   };
   ```

2. **Обновить статистику:**
   ```c
   void simulator_step(Simulator *sim) {
       uint16_t instr = memory_read_halfword(...);
       uint8_t opcode = get_bits(instr, 11, 15);
       
       // ... выполнение ...
       
       stats.cycles += get_instruction_cycles(opcode);
   }
   ```

**Время:** 4-6 часов  
**Сложность:** ⭐⭐

---

### 2.2. Системные тесты (end-to-end)

**Файл:** `test/test_e2e.c`

**Что нужно сделать:**

1. **Тест полного цикла:**
   ```c
   void test_full_cycle() {
       // 1. Создаём задачу
       Task task = {
           .task_id = "test-001",
           .binary = load_file("test_program.bin"),
           .timeout_sec = 5
       };
       
       // 2. Запускаем симулятор
       TaskResult result = run_simulator(&task);
       
       // 3. Проверяем результат
       assert(result.status == STATUS_SUCCESS);
       assert(result.instructions_executed > 0);
       assert(result.gpio_state["R0"] == 5);
   }
   ```

2. **Сравнение с реальным железом:**
   - Взять тестовую программу
   - Запустить на реальном STM32
   - Запустить на симуляторе
   - Сравнить результаты

**Время:** 6-8 часов  
**Сложность:** ⭐⭐⭐

---

### 2.3. OpenTelemetry (метрики)

**Файл:** `orchestrator/main.go`

**Что нужно сделать:**

1. **Добавить метрики:**
   ```go
   import "go.opentelemetry.io/otel"
   
   var (
       taskCounter = metric.Int64Counter("tasks_processed")
       taskDuration = metric.Float64Histogram("task_duration_seconds")
       errorCounter = metric.Int64Counter("task_errors")
   )
   
   func processTask(task Task) {
       startTime := time.Now()
       
       // ... обработка ...
       
       taskCounter.Add(ctx, 1)
       taskDuration.Record(ctx, time.Since(startTime).Seconds())
       
       if result.Status == "error" {
           errorCounter.Add(ctx, 1)
       }
   }
   ```

2. **Настроить экспорт:**
   ```go
   // В main.go
   exporter, _ := stdoutmetric.New()
   otel.SetMeterProvider(
       metric.NewMeterProvider(metric.WithReader(exporter)),
   )
   ```

**Время:** 4-6 часов  
**Сложность:** ⭐⭐⭐

---

## 🟢 Приоритет 3: Опциональные задачи

### 3.1. Расширение набора инструкций

**Файл:** `core/src/execute.c`

**Проверить и добавить:**

- [ ] `BL` (Branch with Link) - вызов функций
- [ ] `BX` (Branch and Exchange) - возврат из функции
- [ ] `CBZ`, `CBNZ` (Compare and Branch)
- [ ] `PUSH`, `POP` (стек)
- [ ] `LDM`, `STM` (multiple load/store)
- [ ] `BFC`, `BFI` (битовые поля)
- [ ] `SBFX`, `UBFX` (битовое извлечение)
- [ ] `SSAT`, `USAT` (насыщение)
- [ ] `CLZ` (count leading zeros)
- [ ] `REV`, `REV16`, `REVSH` (смена порядка байт)

**Время:** 12-16 часов  
**Сложность:** ⭐⭐⭐⭐

---

### 3.2. Презентация проекта

**Структура:**

1. Титульный слайд
2. Проблема и решение (5 мин)
3. Архитектура системы (диаграмма)
4. Реализованные компоненты
5. Демонстрация работы
6. Выводы

**Время:** 3-4 часа  
**Сложность:** ⭐

---

### 3.3. База знаний в Obsidian

**Перенести документацию:**

- `C_GO_COMPATIBILITY.md` → Obsidian
- `IMPLEMENTATION_SUMMARY.md` → Obsidian
- `AUDIT_REPORT.md` → Obsidian
- Добавить ссылки между заметками
- Создать index-заметку

**Время:** 2-3 часа  
**Сложность:** ⭐

---

## 📅 Рекомендуемый план работ

### Неделя 1 (13-19 марта)
- [ ] 1.1 Прерывания (stacking/unstacking)
- [ ] 1.2 TIM6 (счёт + прерывания)
- [ ] 1.3 UART RX

### Неделя 2 (20-26 марта)
- [ ] 1.4 PDF отчёт
- [ ] 2.1 Счётчик тактов (детализация)
- [ ] 2.2 Системные тесты

### Неделя 3 (27 марта - 2 апреля)
- [ ] 2.3 OpenTelemetry
- [ ] 3.1 Расширение инструкций (частично)
- [ ] 3.2 Презентация

### Неделя 4 (3-9 апреля)
- [ ] 3.3 Obsidian
- [ ] Финальное тестирование
- [ ] Подготовка к сдаче

---

## 📊 Прогресс

| Задача | Приоритет | Статус | Прогресс |
|--------|-----------|--------|----------|
| Прерывания | 🔴 | ❌ | 0% |
| TIM6 | 🔴 | ⚠️ | 40% |
| UART RX | 🔴 | ❌ | 0% |
| PDF отчёт | 🔴 | ❌ | 0% |
| Счётчик тактов | 🟡 | ⚠️ | 20% |
| Системные тесты | 🟡 | ❌ | 0% |
| OpenTelemetry | 🟡 | ❌ | 0% |
| Инструкции | 🟢 | ⚠️ | 60% |
| Презентация | 🟢 | ❌ | 0% |
| Obsidian | 🟢 | ⚠️ | 30% |

**Общий прогресс:** 15% от необходимого для 100%

---

## 🎯 Критерии готовности

### Для сдачи на "удовлетворительно" (75%):
- ✅ Ядро работает (инструкции выполняются)
- ✅ GPIO полностью функционален
- ✅ UART передача работает
- ✅ Go-оркестратор работает
- ⚠️ Прерывания (базовая реализация)
- ⚠️ TIM6 (счёт)

### Для сдачи на "хорошо" (85%):
- ✅ Всё из "удовлетворительно"
- ✅ Прерывания полностью работают
- ✅ UART приём/передача
- ✅ Детализированный счётчик тактов
- ✅ Системные тесты
- ✅ PDF отчёт

### Для сдачи на "отлично" (95%+):
- ✅ Всё из "хорошо"
- ✅ OpenTelemetry
- ✅ Расширенный набор инструкций
- ✅ Презентация
- ✅ База знаний Obsidian
- ✅ Сравнение с реальным железом

---

**Следующий шаг:** Начать с реализации прерываний (п. 1.1) — это самая сложная и критичная задача.
