#!/bin/bash
# ============================================================================
# STM32 Simulator - Test Runner
# Автоматический запуск всех тестов симулятора
# ============================================================================
# Использование:
#   ./run_all_tests.sh              - Запустить все тесты
#   ./run_all_tests.sh --verbose    - Подробный вывод
#   ./run_all_tests.sh --quick      - Только итоги
#   ./run_all_tests.sh test_gpio    - Запустить конкретный тест
# ============================================================================

set -e

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Переменные
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
VERBOSE=0
QUICK=0
SPECIFIC_TEST=""

# Счётчики
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Список тестов
TESTS=(
    "test_gpio"
    "test_simulator"
    "test_instructions"
    "test_peripheral"
    "test_extended_instructions"
    "test_peripheral_extended"
    "test_integration"
)

# ============================================================================
# Функции
# ============================================================================

print_header() {
    echo -e "${CYAN}============================================================================${NC}"
    echo -e "${CYAN}  STM32 Simulator - Test Suite${NC}"
    echo -e "${CYAN}============================================================================${NC}"
    echo ""
}

print_usage() {
    echo "Использование:"
    echo "  $0 [OPTIONS] [TEST_NAME]"
    echo ""
    echo "Опции:"
    echo "  --verbose, -v    Подробный вывод всех тестов"
    echo "  --quick, -q      Только итоги (без деталей)"
    echo "  --help, -h       Показать эту справку"
    echo ""
    echo "Примеры:"
    echo "  $0                      # Запустить все тесты"
    echo "  $0 --verbose            # Подробный вывод"
    echo "  $0 test_gpio            # Запустить только test_gpio"
    echo "  $0 -q test_simulator    # Быстрый запуск одного теста"
    echo ""
}

check_build() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo -e "${RED}Error: Build directory not found${NC}"
        echo "Please run: mkdir build && cd build && cmake .. && make"
        exit 1
    fi
    
    # Проверяем наличие хотя бы одного теста
    local first_test="$BUILD_DIR/${TESTS[0]}"
    if [ ! -f "$first_test" ]; then
        echo -e "${RED}Error: Tests not found in $BUILD_DIR${NC}"
        echo "Please build the project first: cd build && make"
        exit 1
    fi
}

run_single_test() {
    local test_name=$1
    local test_path="$BUILD_DIR/$test_name"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ ! -f "$test_path" ]; then
        echo -e "${YELLOW}⊘ SKIPPED: $test_name (not found)${NC}"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
        return 2
    fi
    
    if [ $QUICK -eq 1 ]; then
        # Тихий режим - только результат
        if $test_path > /dev/null 2>&1; then
            echo -e "${GREEN}✓${NC} $test_name"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}✗${NC} $test_name"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        # Подробный режим
        echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
        echo -e "${YELLOW}Running: $test_name${NC}"
        echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
        
        if $test_path; then
            echo -e "${GREEN}✓ PASSED${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}✗ FAILED${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
        echo ""
    fi
}

print_summary() {
    echo ""
    echo -e "${CYAN}============================================================================${NC}"
    echo -e "${CYAN}  Test Summary${NC}"
    echo -e "${CYAN}============================================================================${NC}"
    echo ""
    echo "  Total tests:  $TOTAL_TESTS"
    
    if [ $PASSED_TESTS -gt 0 ]; then
        echo -e "  ${GREEN}Passed:         $PASSED_TESTS${NC}"
    else
        echo -e "  Passed:         $PASSED_TESTS"
    fi
    
    if [ $FAILED_TESTS -gt 0 ]; then
        echo -e "  ${RED}Failed:         $FAILED_TESTS${NC}"
    else
        echo -e "  Failed:         $FAILED_TESTS"
    fi
    
    if [ $SKIPPED_TESTS -gt 0 ]; then
        echo -e "  ${YELLOW}Skipped:        $SKIPPED_TESTS${NC}"
    else
        echo -e "  Skipped:        $SKIPPED_TESTS"
    fi
    
    echo ""
    echo -e "${CYAN}============================================================================${NC}"
    
    # Процент успеха
    if [ $TOTAL_TESTS -gt 0 ]; then
        local success_rate=$(( (PASSED_TESTS * 100) / TOTAL_TESTS ))
        echo ""
        echo -e "Success rate: ${success_rate}%"
        echo ""
    fi
    
    if [ $FAILED_TESTS -eq 0 ] && [ $SKIPPED_TESTS -eq 0 ]; then
        echo -e "${GREEN}🎉 All tests passed!${NC}"
        echo ""
        return 0
    elif [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${YELLOW}⚠️  Some tests were skipped${NC}"
        echo ""
        return 0
    else
        echo -e "${RED}❌ Some tests failed!${NC}"
        echo ""
        return 1
    fi
}

# ============================================================================
# Парсинг аргументов
# ============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --quick|-q)
            QUICK=1
            shift
            ;;
        --help|-h)
            print_header
            print_usage
            exit 0
            ;;
        *)
            SPECIFIC_TEST=$1
            shift
            ;;
    esac
done

# ============================================================================
# Основная логика
# ============================================================================

print_header
check_header=false

if [ -n "$SPECIFIC_TEST" ]; then
    # Запуск конкретного теста
    run_single_test "$SPECIFIC_TEST"
else
    # Запуск всех тестов
    if [ $QUICK -eq 1 ]; then
        echo "Running all tests (quick mode)..."
        echo ""
    fi
    
    for test in "${TESTS[@]}"; do
        run_single_test "$test"
    done
fi

print_summary
exit_code=$?

# Выход с кодом ошибки если тесты упали
exit $exit_code
