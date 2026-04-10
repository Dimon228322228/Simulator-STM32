cmake_minimum_required(VERSION 3.10)
project(STM32_Simulator C)

set(CMAKE_C_STANDARD 11)

# Header search path
include_directories(core/include)

# Core library – all C sources under core/src/
file(GLOB CORE_SOURCES "core/src/*.c")
add_library(core_static STATIC ${CORE_SOURCES})

# Standalone simulator executable
add_executable(stm32_sim core/main.c)
target_link_libraries(stm32_sim core_static)

# Test executables
add_executable(test_gpio                  test/test_gpio.c)
target_link_libraries(test_gpio           core_static)

add_executable(test_simulator             test/test_simulator.c)
target_link_libraries(test_simulator      core_static)

add_executable(test_instructions          test/test_instructions.c)
target_link_libraries(test_instructions   core_static)

add_executable(test_peripheral            test/test_peripheral.c)
target_link_libraries(test_peripheral     core_static)

add_executable(test_extended_instructions test/test_extended_instructions.c)
target_link_libraries(test_extended_instructions core_static)

add_executable(test_peripheral_extended   test/test_peripheral_extended.c)
target_link_libraries(test_peripheral_extended core_static)

add_executable(test_integration           test/test_integration.c)
target_link_libraries(test_integration    core_static)
