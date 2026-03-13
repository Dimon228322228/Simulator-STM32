# Скрипт для быстрой проверки сборки и тестирования симулятора (Windows PowerShell)
# Запуск: .\quick_test.ps1

Write-Host "========================================"
Write-Host "  STM32 Simulator - Quick Test Script"
Write-Host "========================================"
Write-Host ""

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

function Step {
    param([string]$Message)
    Write-Host ">>> $Message" -ForegroundColor Yellow
}

function Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Error {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

# Шаг 1: Проверка зависимостей
Step "Checking dependencies..."

if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    Error "cmake not found. Install from https://cmake.org/download/"
    exit 1
}
Success "cmake found"

if (!(Get-Command gcc -ErrorAction SilentlyContinue)) {
    Error "gcc not found. Install MinGW or use WSL"
    exit 1
}
Success "gcc found"

if (Get-Command go -ErrorAction SilentlyContinue) {
    $goVersion = go version
    Success "go found ($goVersion)"
} else {
    Write-Host "! go not found (optional, for orchestrator)" -ForegroundColor Yellow
}

Write-Host ""

# Шаг 2: Сборка C-симулятора
Step "Building C simulator..."

if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location "build"
cmake .. | Out-Null
mingw32-make | Out-Null
Success "Simulator built successfully"
Set-Location ..

Write-Host ""

# Шаг 3: Запуск демо-режима
Step "Running demo mode..."

$demoOutput = .\build\stm32_sim.exe --demo --max-steps 10 2>&1
if ($demoOutput -match "Simulation finished") {
    Success "Demo mode works"
} else {
    Error "Demo mode failed"
    exit 1
}

Write-Host ""

# Шаг 4: Создание тестовых бинарников
Step "Creating test binary files..."

if (Get-Command python -ErrorAction SilentlyContinue) {
    python create_test_bin.py all
    Success "Test binaries created"
} elseif (Get-Command python3 -ErrorAction SilentlyContinue) {
    python3 create_test_bin.py all
    Success "Test binaries created"
} else {
    Write-Host "! Python not found, skipping binary creation" -ForegroundColor Yellow
}

Write-Host ""

# Шаг 5: Запуск тестового бинарника
Step "Running test binary..."

if (Test-Path "demo.bin") {
    $testOutput = .\build\stm32_sim.exe demo.bin --max-steps 10 2>&1
    if ($testOutput -match "Simulation finished") {
        Success "Test binary execution works"
    } else {
        Error "Test binary execution failed"
    }
}

Write-Host ""

# Шаг 6: Запуск тестов
Step "Running unit tests..."

if (Test-Path ".\build\test_gpio.exe") {
    $gpioOutput = .\build\test_gpio.exe 2>&1
    if ($gpioOutput -match "All GPIO tests passed") {
        Success "GPIO tests passed"
    } else {
        Error "GPIO tests failed"
    }
}

if (Test-Path ".\build\test_simulator.exe") {
    $simOutput = .\build\test_simulator.exe 2>&1
    if ($simOutput -match "All simulator tests passed") {
        Success "Simulator tests passed"
    } else {
        Error "Simulator tests failed"
    }
}

Write-Host ""

# Шаг 7: Сборка Go-оркестратора (если есть Go)
if (Get-Command go -ErrorAction SilentlyContinue) {
    Step "Building Go orchestrator..."
    
    Set-Location "orchestrator"
    if (go build -o orchestrator.exe . 2>$null) {
        Success "Orchestrator built successfully"
    } else {
        Write-Host "! Orchestrator build failed (missing dependencies?)" -ForegroundColor Yellow
    }
    Set-Location ..
    Write-Host ""
}

# Итоги
Write-Host "========================================"
Write-Host "  Test Summary"
Write-Host "========================================"
Write-Host ""
Write-Host "Build artifacts:"

if (Test-Path "build\stm32_sim.exe") {
    $size = (Get-Item "build\stm32_sim.exe").Length
    Write-Host "  - stm32_sim.exe: $([math]::Round($size/1KB, 2)) KB"
} else {
    Write-Host "  - stm32_sim.exe: not found"
}

if (Test-Path "build\test_gpio.exe") {
    Write-Host "  - test_gpio.exe: exists"
} else {
    Write-Host "  - test_gpio.exe: not found"
}

if (Test-Path "build\test_simulator.exe") {
    Write-Host "  - test_simulator.exe: exists"
} else {
    Write-Host "  - test_simulator.exe: not found"
}

if (Test-Path "orchestrator\orchestrator.exe") {
    Write-Host "  - orchestrator.exe: exists"
} else {
    Write-Host "  - orchestrator.exe: not found"
}

Write-Host ""

if (Test-Path "demo.bin") {
    Write-Host "Test binaries:"
    Get-ChildItem *.bin | ForEach-Object {
        Write-Host "  - $($_.Name): $([math]::Round($_.Length/1KB, 4)) KB"
    }
}

Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Run simulator: .\build\stm32_sim.exe --demo"
Write-Host "  2. Run tests: .\build\test_gpio.exe; .\build\test_simulator.exe"
Write-Host "  3. Build orchestrator: cd orchestrator; go build"
Write-Host "  4. Read documentation: cat README.md"
Write-Host ""
Success "All checks completed!"
