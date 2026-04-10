#!/usr/bin/env bash
#
# run_demo.sh — Interactive STM32 Simulator demonstration
#
# Usage: ./run_demo.sh
#
# Presents a menu of demo programs to run in front of an audience.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
SIM="$BUILD_DIR/stm32_sim"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

print_header() {
    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC}  ${BOLD}STM32F103C8T6 Simulator — Demonstration${NC}                  ${CYAN}║${NC}"
    echo -e "${CYAN}║${NC}  Cortex-M3 Thumb-16 Instruction Set Simulator       ${CYAN}║${NC}"
    echo -e "${CYAN}║${NC}  ITMO University — Embedded Systems Programming     ${CYAN}║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

print_section() {
    echo ""
    echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
    echo -e "${BOLD}$1${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
    echo ""
}

build_project() {
    print_section "Step 1: Building the simulator..."

    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
        (cd "$BUILD_DIR" && cmake .. >/dev/null 2>&1)
    fi

    (cd "$BUILD_DIR" && make -j"$(nproc)" >/dev/null 2>&1)

    if [ -f "$SIM" ]; then
        echo -e "  ${GREEN}✓${NC} Simulator built successfully: $SIM"
    else
        echo -e "  ${RED}✗${NC} Build failed!"
        exit 1
    fi
}

generate_demos() {
    print_section "Step 2: Generating demo programs..."

    python3 "$SCRIPT_DIR/demo_programs.py" all
    echo ""
    echo -e "  ${GREEN}✓${NC} All demo binaries generated"
}

run_demo() {
    local name="$1"
    local title="$2"
    local description="$3"
    local expected="$4"

    print_section "Demo: $title"

    echo -e "${BOLD}Description:${NC} $description"
    echo ""
    echo -e "${BOLD}Expected result:${NC}"
    echo -e "  $expected"
    echo ""
    echo -e "${YELLOW}Running: $SIM $name.bin --max-steps 20${NC}"
    echo ""

    "$SIM" "$SCRIPT_DIR/${name}.bin" --max-steps 20

    echo ""
    echo -e "${GREEN}✓${NC} Demo '$title' completed"
}

show_menu() {
    echo -e "${BOLD}Select a demo to run:${NC}"
    echo ""
    echo -e "  ${GREEN}1${NC}) Arithmetic         — ADD, SUB, CMP instructions"
    echo -e "  ${GREEN}2${NC}) Loop Counter       — Count 0→5 with conditional branch"
    echo -e "  ${GREEN}3${NC}) Fibonacci          — Compute 8 Fibonacci numbers"
    echo -e "  ${GREEN}4${NC}) Memory Operations  — STR/LDR to SRAM"
    echo -e "  ${GREEN}5${NC}) LED Blink          — GPIO toggle simulation"
    echo -e "  ${GREEN}6${NC}) Max Value          — Conditional branch (BGT)"
    echo ""
    echo -e "  ${GREEN}a${NC}) Run ALL demos sequentially"
    echo -e "  ${GREEN}t${NC}) Run test suite"
    echo -e "  ${GREEN}q${NC}) Quit"
    echo ""
}

run_all_demos() {
    run_demo "arithmetic"    "Arithmetic Operations" \
        "Executes ADD, SUB, CMP on two values" \
        "R0=10, R1=3, R2=13 (sum), R3=7 (diff)"

    run_demo "loop"          "Loop Counter (0→5)" \
        "Counts from 0 to 5 using BLE conditional branch" \
        "R0=6 (final count after loop exit)"

    run_demo "fibonacci"     "Fibonacci Sequence" \
        "Computes 8 Fibonacci numbers in registers" \
        "R0=8, R1=13 (8th and 9th Fibonacci numbers)"

    run_demo "memory"        "Memory Operations" \
        "Stores and loads values from SRAM" \
        "Data written to SRAM and read back"

    run_demo "led_blink"     "LED Blink Simulation" \
        "Toggles GPIO output bits in a loop" \
        "R2 toggles between 0 and 1"

    run_demo "max_value"     "Find Maximum Value" \
        "Compares two values using BGT conditional branch" \
        "R2=42 (the larger of 42 and 17)"
}

run_tests() {
    print_section "Running full test suite..."
    "$SCRIPT_DIR/run_tests.sh"
}

# ── Main ──────────────────────────────────────────────────────────────

print_header

# Build and generate
build_project
generate_demos

# Interactive menu
while true; do
    show_menu
    read -rp "Your choice: " choice

    case "$choice" in
        1) run_demo "arithmetic" "Arithmetic Operations" \
               "Executes ADD, SUB, CMP on two values" \
               "R0=10, R1=3, R2=13 (sum), R3=7 (diff)" ;;
        2) run_demo "loop" "Loop Counter (0→5)" \
               "Counts from 0 to 5 using BLE conditional branch" \
               "R0=6 (final count after loop exit)" ;;
        3) run_demo "fibonacci" "Fibonacci Sequence" \
               "Computes 8 Fibonacci numbers in registers" \
               "R0=8, R1=13 (8th and 9th Fibonacci numbers)" ;;
        4) run_demo "memory" "Memory Operations" \
               "Stores and loads values from SRAM" \
               "Data written to SRAM and read back" ;;
        5) run_demo "led_blink" "LED Blink Simulation" \
               "Toggles GPIO output bits in a loop" \
               "R2 toggles between 0 and 1" ;;
        6) run_demo "max_value" "Find Maximum Value" \
               "Compares two values using BGT conditional branch" \
               "R2=42 (the larger of 42 and 17)" ;;
        a) run_all_demos ;;
        t) run_tests ;;
        q|Q) echo -e "\n${CYAN}Goodbye!${NC}"; exit 0 ;;
        *) echo -e "\n${RED}Invalid choice. Try again.${NC}" ;;
    esac

    echo ""
    read -rp "Press Enter to continue..."
done
