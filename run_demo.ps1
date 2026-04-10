# run_demo.ps1 — Interactive STM32 Simulator demonstration (Windows PowerShell)
#
# Usage: .\run_demo.ps1
#

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ScriptDir "build"
$Sim = Join-Path $BuildDir "stm32_sim.exe"

function Print-Header {
    Write-Host ""
    Write-Host "========================================================" -ForegroundColor Cyan
    Write-Host "  STM32F103C8T6 Simulator — Demonstration" -ForegroundColor Cyan
    Write-Host "  Cortex-M3 Thumb-16 Instruction Set Simulator" -ForegroundColor Cyan
    Write-Host "  ITMO University — Embedded Systems Programming" -ForegroundColor Cyan
    Write-Host "========================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Print-Section {
    param([string]$Title)
    Write-Host ""
    Write-Host "────────────────────────────────────────────────────────" -ForegroundColor Blue
    Write-Host "  $Title" -ForegroundColor White -BackgroundColor Blue
    Write-Host "────────────────────────────────────────────────────────" -ForegroundColor Blue
    Write-Host ""
}

function Build-Project {
    Print-Section "Step 1: Building the simulator..."

    if (!(Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
        Push-Location $BuildDir
        cmake .. -G "MinGW Makefiles" 2>&1 | Out-Null
        Pop-Location
    }

    Push-Location $BuildDir
    mingw32-make -j4 2>&1 | Out-Null
    Pop-Location

    if (Test-Path $Sim) {
        Write-Host "  [OK] Simulator built successfully" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] Build failed!" -ForegroundColor Red
        exit 1
    }
}

function Generate-Demos {
    Print-Section "Step 2: Generating demo programs..."

    python3 "$ScriptDir\demo_programs.py" all
    Write-Host ""
    Write-Host "  [OK] All demo binaries generated" -ForegroundColor Green
}

function Run-Demo {
    param(
        [string]$Name,
        [string]$Title,
        [string]$Description,
        [string]$Expected
    )

    Print-Section "Demo: $Title"

    Write-Host "Description: $Description" -ForegroundColor White
    Write-Host ""
    Write-Host "Expected result:" -ForegroundColor Yellow
    Write-Host "  $Expected" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Running: $Sim $Name.bin --max-steps 20" -ForegroundColor Gray
    Write-Host ""

    $binPath = Join-Path $ScriptDir "$Name.bin"
    & $Sim $binPath --max-steps 20

    Write-Host ""
    Write-Host "  [OK] Demo '$Title' completed" -ForegroundColor Green
}

function Show-Menu {
    Write-Host ""
    Write-Host "Select a demo to run:" -ForegroundColor White -BackgroundColor DarkBlue
    Write-Host ""
    Write-Host "  1) Arithmetic         — ADD, SUB, CMP instructions" -ForegroundColor Green
    Write-Host "  2) Loop Counter       — Count 0 to 5 with conditional branch" -ForegroundColor Green
    Write-Host "  3) Fibonacci          — Compute 8 Fibonacci numbers" -ForegroundColor Green
    Write-Host "  4) Memory Operations  — STR/LDR to SRAM" -ForegroundColor Green
    Write-Host "  5) LED Blink          — GPIO toggle simulation" -ForegroundColor Green
    Write-Host "  6) Max Value          — Conditional branch (BGT)" -ForegroundColor Green
    Write-Host ""
    Write-Host "  a) Run ALL demos sequentially" -ForegroundColor Cyan
    Write-Host "  t) Run test suite" -ForegroundColor Cyan
    Write-Host "  q) Quit" -ForegroundColor Red
    Write-Host ""
}

function Run-All-Demos {
    Run-Demo "arithmetic" "Arithmetic Operations" `
        "Executes ADD, SUB, CMP on two values" `
        "R0=10, R1=3, R2=13 (sum), R3=7 (diff)"

    Run-Demo "loop" "Loop Counter (0 to 5)" `
        "Counts from 0 to 5 using BLE conditional branch" `
        "R0=6 (final count after loop exit)"

    Run-Demo "fibonacci" "Fibonacci Sequence" `
        "Computes 8 Fibonacci numbers in registers" `
        "R0=8, R1=13 (8th and 9th Fibonacci numbers)"

    Run-Demo "memory" "Memory Operations" `
        "Stores and loads values from SRAM" `
        "Data written to SRAM and read back"

    Run-Demo "led_blink" "LED Blink Simulation" `
        "Toggles GPIO output bits in a loop" `
        "R2 toggles between 0 and 1"

    Run-Demo "max_value" "Find Maximum Value" `
        "Compares two values using BGT conditional branch" `
        "R2=42 (the larger of 42 and 17)"
}

function Run-Tests {
    Print-Section "Running full test suite..."
    & "$ScriptDir\run_tests.ps1"
}

# ── Main ──────────────────────────────────────────────────────────────

Print-Header
Build-Project
Generate-Demos

while ($true) {
    Show-Menu
    $choice = Read-Host "Your choice"

    switch ($choice) {
        "1" { Run-Demo "arithmetic" "Arithmetic Operations" `
              "Executes ADD, SUB, CMP on two values" `
              "R0=10, R1=3, R2=13 (sum), R3=7 (diff)" }
        "2" { Run-Demo "loop" "Loop Counter (0 to 5)" `
              "Counts from 0 to 5 using BLE conditional branch" `
              "R0=6 (final count after loop exit)" }
        "3" { Run-Demo "fibonacci" "Fibonacci Sequence" `
              "Computes 8 Fibonacci numbers in registers" `
              "R0=8, R1=13 (8th and 9th Fibonacci numbers)" }
        "4" { Run-Demo "memory" "Memory Operations" `
              "Stores and loads values from SRAM" `
              "Data written to SRAM and read back" }
        "5" { Run-Demo "led_blink" "LED Blink Simulation" `
              "Toggles GPIO output bits in a loop" `
              "R2 toggles between 0 and 1" }
        "6" { Run-Demo "max_value" "Find Maximum Value" `
              "Compares two values using BGT conditional branch" `
              "R2=42 (the larger of 42 and 17)" }
        "a" { Run-All-Demos }
        "t" { Run-Tests }
        "q" { Write-Host "`nGoodbye!" -ForegroundColor Cyan; exit 0 }
        default { Write-Host "`nInvalid choice. Try again." -ForegroundColor Red }
    }

    Write-Host ""
    Read-Host "Press Enter to continue"
}
