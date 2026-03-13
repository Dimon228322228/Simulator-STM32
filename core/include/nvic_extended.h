#ifndef NVIC_EXTENDED_H
#define NVIC_EXTENDED_H

#include <stdint.h>

// IRQ номера для STM32F103C8T6
#define NVIC_IRQ_NUMBER         80

// Векторы прерываний (примеры)
#define NVIC_IRQ_WWDG           0
#define NVIC_IRQ_PVD            1
#define NVIC_IRQ_TAMPER         2
#define NVIC_IRQ_RTC            3
#define NVIC_IRQ_FLASH          4
#define NVIC_IRQ_RCC            5
#define NVIC_IRQ_EXTI0          6
#define NVIC_IRQ_EXTI1          7
#define NVIC_IRQ_EXTI2          8
#define NVIC_IRQ_EXTI3          9
#define NVIC_IRQ_EXTI4          10
#define NVIC_IRQ_DMA1_CHANNEL1  11
#define NVIC_IRQ_DMA1_CHANNEL2  12
#define NVIC_IRQ_DMA1_CHANNEL3  13
#define NVIC_IRQ_DMA1_CHANNEL4  14
#define NVIC_IRQ_DMA1_CHANNEL5  15
#define NVIC_IRQ_DMA1_CHANNEL6  16
#define NVIC_IRQ_DMA1_CHANNEL7  17
#define NVIC_IRQ_ADC            18
#define NVIC_IRQ_USB_HP_CAN_TX  19
#define NVIC_IRQ_USB_LP_CAN_RX0 20
#define NVIC_IRQ_CAN_RX1        21
#define NVIC_IRQ_CAN_SCE        22
#define NVIC_IRQ_TIM1_BRK       23
#define NVIC_IRQ_TIM1_UP        24
#define NVIC_IRQ_TIM1_TRG_COM   25
#define NVIC_IRQ_TIM1_CC        26
#define NVIC_IRQ_TIM2           27
#define NVIC_IRQ_TIM3           28
#define NVIC_IRQ_TIM4           29
#define NVIC_IRQ_I2C1_EV        31
#define NVIC_IRQ_I2C1_ER        32
#define NVIC_IRQ_I2C2_EV        33
#define NVIC_IRQ_I2C2_ER        34
#define NVIC_IRQ_SPI1           35
#define NVIC_IRQ_SPI2           36
#define NVIC_IRQ_USART1         37
#define NVIC_IRQ_USART2         38
#define NVIC_IRQ_USART3         39
#define NVIC_IRQ_EXTI9_5        40
#define NVIC_IRQ_TIM6           41
#define NVIC_IRQ_TIM7           42
#define NVIC_IRQ_TIM8_BRK       43
#define NVIC_IRQ_TIM8_UP        44
#define NVIC_IRQ_TIM8_TRG_COM   45
#define NVIC_IRQ_TIM8_CC        46
#define NVIC_IRQ_ADC3           47
#define NVIC_IRQ_FSMC           48
#define NVIC_IRQ_SDIO           49
#define NVIC_IRQ_TIM5           50
#define NVIC_IRQ_SPI3           51
#define NVIC_IRQ_UART4          52
#define NVIC_IRQ_UART5          53
#define NVIC_IRQ_TIM6_DAC       54
#define NVIC_IRQ_TIM7_DAC       55
#define NVIC_IRQ_DMA2_CHANNEL1  56
#define NVIC_IRQ_DMA2_CHANNEL2  57
#define NVIC_IRQ_DMA2_CHANNEL3  58
#define NVIC_IRQ_DMA2_CHANNEL4_5 59

// Структура для хранения информации о векторе прерывания
typedef struct {
    uint32_t vector_address;    // Адрес вектора прерывания
    uint8_t priority;           // Приоритет прерывания
    uint8_t enabled;            // Флаг включения прерывания
    uint8_t pending;            // Флаг ожидающего прерывания
    uint8_t active;             // Флаг активного прерывания
} NVIC_Vector;

// Расширенная структура NVIC
typedef struct {
    NVIC_Vector vectors[NVIC_IRQ_NUMBER];  // Векторы прерываний
    uint32_t base_priority;                // Базовый приоритет
    uint32_t active_priority;              // Активный приоритет
    uint32_t interrupt_active;             // Активные прерывания
    uint32_t interrupt_pending;            // Ожидающие прерывания
    uint32_t interrupt_enabled;            // Включенные прерывания
} NVIC_Extended_State;

// Инициализация расширенного NVIC
void nvic_extended_init(NVIC_Extended_State *nvic);

// Включение прерывания
void nvic_enable_irq(NVIC_Extended_State *nvic, uint8_t irqn);

// Выключение прерывания
void nvic_disable_irq(NVIC_Extended_State *nvic, uint8_t irqn);

// Установка приоритета прерывания
void nvic_set_priority(NVIC_Extended_State *nvic, uint8_t irqn, uint8_t priority);

// Получение приоритета прерывания
uint8_t nvic_get_priority(NVIC_Extended_State *nvic, uint8_t irqn);

// Установка прерывания как ожидающего
void nvic_set_pending(NVIC_Extended_State *nvic, uint8_t irqn);

// Сброс прерывания как ожидающего
void nvic_clear_pending(NVIC_Extended_State *nvic, uint8_t irqn);

// Проверка, активно ли прерывание
uint8_t nvic_is_active(NVIC_Extended_State *nvic, uint8_t irqn);

// Проверка, включено ли прерывание
uint8_t nvic_is_enabled(NVIC_Extended_State *nvic, uint8_t irqn);

// Проверка, ожидает ли прерывание
uint8_t nvic_is_pending(NVIC_Extended_State *nvic, uint8_t irqn);

// Обработка прерывания (основная логика)
void nvic_handle_interrupt(NVIC_Extended_State *nvic, uint8_t irqn);

// Проверка наличия активных прерываний
uint8_t nvic_has_active_interrupt(NVIC_Extended_State *nvic);

// Получение номера активного прерывания
uint8_t nvic_get_active_interrupt(NVIC_Extended_State *nvic);

#endif // NVIC_EXTENDED_H