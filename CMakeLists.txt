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
    "core/src/bus_matrix.c"
    "core/src/rcc.c"
    "core/src/dma.c"
)

# Создаем библиотеку ядра (чтобы потом линковать с Go или тестами)
add_library(core_static STATIC ${CORE_SOURCES})

# Создаем исполняемый файл для отладки ядра (standalone)
add_executable(stm32_sim core/main.c)
target_link_libraries(stm32_sim core_static)

# Тесты
add_executable(test_gpio test/test_gpio.c $<TARGET_OBJECTS:core_static>)
add_executable(test_simulator test/test_simulator.c $<TARGET_OBJECTS:core_static>)
add_executable(test_instructions test/test_instructions.c $<TARGET_OBJECTS:core_static>)
add_executable(test_peripheral test/test_peripheral.c $<TARGET_OBJECTS:core_static>)

# qwen code к vs code подключить
# router ai.ru
# qwen coder next
