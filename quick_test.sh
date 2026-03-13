#!/bin/bash
# Скрипт для быстрой проверки сборки и тестирования симулятора
# Работает в Linux/WSL

set -e

echo "========================================"
echo "  STM32 Simulator - Quick Test Script"
echo "========================================"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Цвета для вывода
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

step() {
    echo -e "${YELLOW}>>> $1${NC}"
}

success() {
    echo -e "${GREEN}✓ $1${NC}"
}

error() {
    echo -e "${RED}✗ $1${NC}"
}

# Шаг 1: Проверка зависимостей
step "Checking dependencies..."

if ! command -v cmake &> /dev/null; then
    error "cmake not found. Install with: apt install cmake"
    exit 1
fi
success "cmake found"

if ! command -v make &> /dev/null; then
    error "make not found. Install with: apt install make"
    exit 1
fi
success "make found"

if ! command -v gcc &> /dev/null; then
    error "gcc not found. Install with: apt install gcc"
    exit 1
fi
success "gcc found"

if command -v go &> /dev/null; then
    success "go found ($(go version))"
else
    echo -e "${YELLOW}! go not found (optional, for orchestrator)${NC}"
fi

echo ""

# Шаг 2: Сборка C-симулятора
step "Building C simulator..."

mkdir -p build
cd build
cmake .. > /dev/null
make > /dev/null
success "Simulator built successfully"

cd ..
echo ""

# Шаг 3: Запуск демо-режима
step "Running demo mode..."

if ./build/stm32_sim --demo --max-steps 10 2>&1 | grep -q "Simulation finished"; then
    success "Demo mode works"
else
    error "Demo mode failed"
    exit 1
fi
echo ""

# Шаг 4: Создание тестовых бинарников
step "Creating test binary files..."

if command -v python3 &> /dev/null; then
    python3 create_test_bin.py all
    success "Test binaries created"
else
    echo -e "${YELLOW}! Python3 not found, skipping binary creation${NC}"
fi
echo ""

# Шаг 5: Запуск тестового бинарника
step "Running test binary..."

if [ -f "demo.bin" ]; then
    if ./build/stm32_sim demo.bin --max-steps 10 2>&1 | grep -q "Simulation finished"; then
        success "Test binary execution works"
    else
        error "Test binary execution failed"
    fi
fi
echo ""

# Шаг 6: Запуск тестов
step "Running unit tests..."

if [ -f "./build/test_gpio" ]; then
    if ./build/test_gpio 2>&1 | grep -q "All GPIO tests passed"; then
        success "GPIO tests passed"
    else
        error "GPIO tests failed"
    fi
fi

if [ -f "./build/test_simulator" ]; then
    if ./build/test_simulator 2>&1 | grep -q "All simulator tests passed"; then
        success "Simulator tests passed"
    else
        error "Simulator tests failed"
    fi
fi
echo ""

# Шаг 7: Сборка Go-оркестратора (если есть Go)
if command -v go &> /dev/null; then
    step "Building Go orchestrator..."
    
    cd orchestrator
    if go build -o orchestrator . 2>/dev/null; then
        success "Orchestrator built successfully"
    else
        echo -e "${YELLOW}! Orchestrator build failed (missing dependencies?)${NC}"
    fi
    cd ..
    echo ""
fi

# Итоги
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo ""
echo "Build artifacts:"
ls -lh build/stm32_sim 2>/dev/null || echo "  - stm32_sim: not found"
ls -lh build/test_gpio 2>/dev/null || echo "  - test_gpio: not found"
ls -lh build/test_simulator 2>/dev/null || echo "  - test_simulator: not found"
ls -lh orchestrator/orchestrator 2>/dev/null || echo "  - orchestrator: not found"
echo ""

if [ -f "demo.bin" ]; then
    echo "Test binaries:"
    ls -lh *.bin 2>/dev/null
fi

echo ""
echo "Next steps:"
echo "  1. Run simulator: ./build/stm32_sim --demo"
echo "  2. Run tests: ./build/test_gpio && ./build/test_simulator"
echo "  3. Build orchestrator: cd orchestrator && go build"
echo "  4. Read documentation: cat README.md"
echo ""
success "All checks completed!"
