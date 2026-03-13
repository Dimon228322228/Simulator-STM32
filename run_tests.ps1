# Скрипт для запуска всех тестов симулятора (Windows PowerShell)

Write-Host "========================================"
Write-Host "  STM32 Simulator - Test Suite"
Write-Host "========================================"
Write-Host ""

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

# Счётчики
$TotalTests = 0
$PassedTests = 0
$FailedTests = 0

function Run-Test {
    param(
        [string]$TestName,
        [string]$TestPath
    )
    
    $script:TotalTests++
    
    Write-Host "Running: $TestName" -ForegroundColor Yellow
    Write-Host "----------------------------------------"
    
    $process = Start-Process -FilePath $TestPath -PassThru -Wait -NoNewWindow
    $process.WaitForExit()
    
    if ($process.ExitCode -eq 0) {
        Write-Host "✓ PASSED" -ForegroundColor Green
        $script:PassedTests++
    } else {
        Write-Host "✗ FAILED (Exit code: $($process.ExitCode))" -ForegroundColor Red
        $script:FailedTests++
    }
    
    Write-Host ""
}

# Проверяем, что сборка выполнена
if (!(Test-Path "build")) {
    Write-Host "Error: build directory not found" -ForegroundColor Red
    Write-Host "Please run: mkdir build; cd build; cmake ..; make"
    exit 1
}

Set-Location "build"

# Список тестов
$Tests = @(
    "test_gpio.exe",
    "test_simulator.exe",
    "test_instructions.exe",
    "test_peripheral.exe",
    "test_extended_instructions.exe",
    "test_peripheral_extended.exe",
    "test_integration.exe"
)

Write-Host "Found $($Tests.Count) test executables"
Write-Host ""

# Запускаем тесты
foreach ($test in $Tests) {
    if (Test-Path $test) {
        $testName = $test -replace "\.exe$", ""
        Run-Test $testName ".\$test"
    } else {
        Write-Host "Warning: $test not found, skipping" -ForegroundColor Yellow
        Write-Host ""
    }
}

# Итоги
Write-Host "========================================"
Write-Host "  Test Summary"
Write-Host "========================================"
Write-Host ""
Write-Host "Total tests:  $TotalTests"
Write-Host "Passed:       $PassedTests" -ForegroundColor Green
Write-Host "Failed:       $FailedTests" -ForegroundColor Red
Write-Host ""

if ($FailedTests -eq 0) {
    Write-Host "All tests passed!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Some tests failed!" -ForegroundColor Red
    exit 1
}
