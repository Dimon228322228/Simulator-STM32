#!/bin/bash
# Скрипт для запуска всех тестов симулятора
# Работает в Linux/WSL

set -e

echo "========================================"
echo "  STM32 Simulator - Test Suite"
echo "========================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Цвета для вывода
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Счётчики
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

run_test() {
    local test_name=$1
    local test_path=$2
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -e "${YELLOW}Running: $test_name${NC}"
    echo "----------------------------------------"
    
    if $test_path; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}✗ FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    echo ""
}

# Проверяем, что сборка выполнена
if [ ! -d "build" ]; then
    echo -e "${RED}Error: build directory not found${NC}"
    echo "Please run: mkdir build && cd build && cmake .. && make"
    exit 1
fi

cd build

# Проверяем наличие тестов
TESTS=(
    "test_gpio"
    "test_simulator"
    "test_instructions"
    "test_peripheral"
    "test_extended_instructions"
    "test_peripheral_extended"
    "test_integration"
)

echo "Found ${#TESTS[@]} test executables"
echo ""

# Запускаем тесты
for test in "${TESTS[@]}"; do
    if [ -f "./$test" ]; then
        run_test "$test" "./$test"
    else
        echo -e "${YELLOW}Warning: $test not found, skipping${NC}"
        echo ""
    fi
done

# Итоги
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo ""
echo "Total tests:  $TOTAL_TESTS"
echo -e "Passed:         ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed:         ${RED}$FAILED_TESTS${NC}"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
