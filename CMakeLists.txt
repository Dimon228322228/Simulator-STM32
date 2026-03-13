cmake_minimum_required(VERSION 3.10)
project(STM32_Simulator C)

set(CMAKE_C_STANDARD 11)

# Включаем директории с заголовками
include_directories(core/include)

# Добавляем исходные файлы ядра
file(GLOB CORE_SOURCES "core/src/*.c")

# Добавляем новые файлы периферии
list(APPEND CORE_SOURCES
    "core/src/tim6.c"
    "core/src/nvic.c"
    "core/src/nvic_extended.c"
    "core/src/bus_matrix.c"
    "core/src/rcc.c"
    "core/src/dma.c"
    "core/src/usart.c"
    "core/src/spi.c"
    "core/src/i2c.c"
)

# Создаем библиотеку ядра (чтобы потом линковать с Go или тестами)
add_library(core_static STATIC ${CORE_SOURCES})

# Создаем исполняемый файл для отладки ядра (standalone)
add_executable(stm32_sim core/main.c)
target_link_libraries(stm32_sim core_static)

# Тесты
add_executable(test_gpio test/test_gpio.c)
target_link_libraries(test_gpio core_static)

add_executable(test_simulator test/test_simulator.c)
target_link_libraries(test_simulator core_static)

add_executable(test_instructions test/test_instructions.c)
target_link_libraries(test_instructions core_static)

add_executable(test_peripheral test/test_peripheral.c)
target_link_libraries(test_peripheral core_static)

# Новые расширенные тесты
add_executable(test_extended_instructions test/test_extended_instructions.c)
target_link_libraries(test_extended_instructions core_static)

add_executable(test_peripheral_extended test/test_peripheral_extended.c)
target_link_libraries(test_peripheral_extended core_static)

add_executable(test_integration test/test_integration.c)
target_link_libraries(test_integration core_static)
