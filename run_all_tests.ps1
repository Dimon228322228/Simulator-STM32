# ============================================================================
# STM32 Simulator - Test Runner (PowerShell)
# Автоматический запуск всех тестов симулятора
# ============================================================================
# Использование:
#   .\run_all_tests.ps1              - Запустить все тесты
#   .\run_all_tests.ps1 -Verbose     - Подробный вывод
#   .\run_all_tests.ps1 -Quick       - Только итоги
#   .\run_all_tests.ps1 test_gpio    - Запустить конкретный тест
# ============================================================================

[CmdletBinding()]
param(
    [switch]$Verbose,
    [switch]$Quick,
    [switch]$Help,
    [string]$TestName
)

# ============================================================================
# Переменные
# ============================================================================
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ScriptDir "build"

# Счётчики
$TotalTests = 0
$PassedTests = 0
$FailedTests = 0
$SkippedTests = 0

# Список тестов
$Tests = @(
    "test_gpio",
    "test_simulator",
    "test_instructions",
    "test_peripheral",
    "test_extended_instructions",
    "test_peripheral_extended",
    "test_integration"
)

# ============================================================================
# Функции
# ============================================================================

function Print-Header {
    Write-Host "============================================================================" -ForegroundColor Cyan
    Write-Host "  STM32 Simulator - Test Suite" -ForegroundColor Cyan
    Write-Host "============================================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Print-Usage {
    Write-Host "Использование:"
    Write-Host "  .\run_all_tests.ps1 [OPTIONS] [TEST_NAME]"
    Write-Host ""
    Write-Host "Опции:"
    Write-Host "  -Verbose         Подробный вывод всех тестов"
    Write-Host "  -Quick           Только итоги (без деталей)"
    Write-Host "  -Help            Показать эту справку"
    Write-Host ""
    Write-Host "Примеры:"
    Write-Host "  .\run_all_tests.ps1                # Запустить все тесты"
    Write-Host "  .\run_all_tests.ps1 -Verbose       # Подробный вывод"
    Write-Host "  .\run_all_tests.ps1 test_gpio      # Запустить только test_gpio"
    Write-Host "  .\run_all_tests.ps1 -Quick         # Быстрый режим"
    Write-Host ""
}

function Check-Build {
    if (!(Test-Path $BuildDir)) {
        Write-Host "Error: Build directory not found" -ForegroundColor Red
        Write-Host "Please run: mkdir build; cd build; cmake ..; make"
        exit 1
    }
    
    # Проверяем наличие хотя бы одного теста
    $firstTest = Join-Path $BuildDir $Tests[0]
    if (!(Test-Path $firstTest)) {
        Write-Host "Error: Tests not found in $BuildDir" -ForegroundColor Red
        Write-Host "Please build the project first: cd build; make"
        exit 1
    }
}

function Run-SingleTest {
    param([string]$Test)
    
    $script:TotalTests++
    
    $testPath = Join-Path $BuildDir $Test
    $testExe = if ($Test -notlike "*.exe") { "$testPath.exe" } else { $testPath }
    
    if (!(Test-Path $testExe)) {
        Write-Host "⊘ SKIPPED: $Test (not found)" -ForegroundColor Yellow
        $script:SkippedTests++
        return
    }
    
    if ($Quick) {
        # Тихий режим - только результат
        $process = Start-Process -FilePath $testExe -PassThru -Wait -NoNewWindow
        if ($process.ExitCode -eq 0) {
            Write-Host "✓ $Test" -ForegroundColor Green
            $script:PassedTests++
        } else {
            Write-Host "✗ $Test" -ForegroundColor Red
            $script:FailedTests++
        }
    } else {
        # Подробный режим
        Write-Host "────────────────────────────────────────────────────────────────" -ForegroundColor Blue
        Write-Host "Running: $Test" -ForegroundColor Yellow
        Write-Host "────────────────────────────────────────────────────────────────" -ForegroundColor Blue
        
        $process = Start-Process -FilePath $testExe -PassThru -Wait -NoNewWindow
        if ($process.ExitCode -eq 0) {
            Write-Host "✓ PASSED" -ForegroundColor Green
            $script:PassedTests++
        } else {
            Write-Host "✗ FAILED" -ForegroundColor Red
            $script:FailedTests++
        }
        Write-Host ""
    }
}

function Print-Summary {
    Write-Host ""
    Write-Host "============================================================================" -ForegroundColor Cyan
    Write-Host "  Test Summary" -ForegroundColor Cyan
    Write-Host "============================================================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Total tests:  $TotalTests"
    
    if ($PassedTests -gt 0) {
        Write-Host "  Passed:         $PassedTests" -ForegroundColor Green
    } else {
        Write-Host "  Passed:         $PassedTests"
    }
    
    if ($FailedTests -gt 0) {
        Write-Host "  Failed:         $FailedTests" -ForegroundColor Red
    } else {
        Write-Host "  Failed:         $FailedTests"
    }
    
    if ($SkippedTests -gt 0) {
        Write-Host "  Skipped:        $SkippedTests" -ForegroundColor Yellow
    } else {
        Write-Host "  Skipped:        $SkippedTests"
    }
    
    Write-Host ""
    Write-Host "============================================================================" -ForegroundColor Cyan
    
    # Процент успеха
    if ($TotalTests -gt 0) {
        $successRate = [math]::Floor(($PassedTests * 100) / $TotalTests)
        Write-Host ""
        Write-Host "Success rate: ${successRate}%"
        Write-Host ""
    }
    
    if ($FailedTests -eq 0 -and $SkippedTests -eq 0) {
        Write-Host "🎉 All tests passed!" -ForegroundColor Green
        Write-Host ""
        return 0
    } elseif ($FailedTests -eq 0) {
        Write-Host "⚠️  Some tests were skipped" -ForegroundColor Yellow
        Write-Host ""
        return 0
    } else {
        Write-Host "❌ Some tests failed!" -ForegroundColor Red
        Write-Host ""
        return 1
    }
}

# ============================================================================
# Основная логика
# ============================================================================

if ($Help) {
    Print-Header
    Print-Usage
    exit 0
}

Print-Header
Check-Build

if ($TestName) {
    # Запуск конкретного теста
    Run-SingleTest -Test $TestName
} else {
    # Запуск всех тестов
    if ($Quick) {
        Write-Host "Running all tests (quick mode)..."
        Write-Host ""
    }
    
    foreach ($test in $Tests) {
        Run-SingleTest -Test $test
    }
}

Print-Summary
exit $LASTEXITCODE
