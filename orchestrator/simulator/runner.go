package simulator

import (
	"context"
	"encoding/base64"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"time"

	"stm32-orchestrator/types"
)

// Runner отвечает за запуск симулятора
type Runner struct {
	simulatorPath string
	timeout       time.Duration
}

// NewRunner создаёт новый Runner
func NewRunner(simulatorPath string, defaultTimeout time.Duration) *Runner {
	return &Runner{
		simulatorPath: simulatorPath,
		timeout:       defaultTimeout,
	}
}

// RunTask выполняет задачу симуляции
func (r *Runner) RunTask(ctx context.Context, task *types.Task) (*types.TaskResult, error) {
	result := &types.TaskResult{
		TaskID:    task.TaskID,
		GPIOState: make(map[string]uint32),
	}

	// Декодируем бинарник из base64
	binaryData, err := base64.StdEncoding.DecodeString(task.Binary)
	if err != nil {
		result.Status = "error"
		result.ErrorMessage = fmt.Sprintf("Failed to decode binary: %v", err)
		return result, nil
	}

	// Создаём временный файл с прошивкой
	tmpFile, err := os.CreateTemp("", "firmware_*.bin")
	if err != nil {
		result.Status = "error"
		result.ErrorMessage = fmt.Sprintf("Failed to create temp file: %v", err)
		return result, nil
	}
	tmpFilename := tmpFile.Name()
	defer os.Remove(tmpFilename)

	// Записываем бинарник
	if _, err := tmpFile.Write(binaryData); err != nil {
		tmpFile.Close()
		result.Status = "error"
		result.ErrorMessage = fmt.Sprintf("Failed to write firmware: %v", err)
		return result, nil
	}
	tmpFile.Close()

	log.Printf("[INFO] Created temp firmware file: %s (%d bytes)", tmpFilename, len(binaryData))

	// Определяем таймаут
	execTimeout := r.timeout
	if task.TimeoutSec > 0 {
		execTimeout = time.Duration(task.TimeoutSec) * time.Second
	}

	// Запускаем симулятор
	cmd := exec.CommandContext(ctx, r.simulatorPath, "--max-steps", "10000", tmpFilename)
	cmd.Dir = filepath.Dir(r.simulatorPath)

	// Запускаем с таймаутом
	done := make(chan error, 1)
	var stdout strings.Builder

	cmd.Stdout = &stdout
	cmd.Stderr = &stdout

	startTime := time.Now()

	if err := cmd.Start(); err != nil {
		result.Status = "error"
		result.ErrorMessage = fmt.Sprintf("Failed to start simulator: %v", err)
		return result, nil
	}

	go func() {
		done <- cmd.Wait()
	}()

	select {
	case <-time.After(execTimeout):
		// Таймаут - убиваем процесс
		cmd.Process.Kill()
		result.Status = "timeout"
		result.ErrorMessage = fmt.Sprintf("Execution time exceeded timeout (%v)", execTimeout)
		log.Printf("[WARN] Task %s timed out after %v", task.TaskID, execTimeout)

	case err := <-done:
		duration := time.Since(startTime)
		result.Stdout = stdout.String()

		if err != nil {
			if exitErr, ok := err.(*exec.ExitError); ok {
				log.Printf("[WARN] Simulator exited with error: %v", exitErr)
			}
		}

		// Парсим вывод симулятора
		r.parseOutput(result, stdout.String())

		if result.Status == "" {
			result.Status = "success"
		}

		log.Printf("[INFO] Task %s completed in %v", task.TaskID, duration)
	}

	return result, nil
}

// parseOutput парсит вывод симулятора и извлекает результаты
func (r *Runner) parseOutput(result *types.TaskResult, output string) {
	lines := strings.Split(output, "\n")

	// Регулярные выражения для парсинга
	statsRegex := regexp.MustCompile(`\[STATS\] Instructions executed: (\d+)`)
	cyclesRegex := regexp.MustCompile(`\[STATS\] CPU cycles: (\d+)`)
	gpioRegex := regexp.MustCompile(`\[GPIO\] PORT_(\w)_ODR = 0x([0-9A-Fa-f]+)`)
	uartRegex := regexp.MustCompile(`\[UART\] TX: 0x([0-9A-Fa-f]{2})`)
	
	// Регулярки для парсинга регистров из вывода CPU
	regR0Regex := regexp.MustCompile(`R0:\s+0x([0-9A-Fa-f]+)`)
	regR1Regex := regexp.MustCompile(`R1:\s+0x([0-9A-Fa-f]+)`)
	regR2Regex := regexp.MustCompile(`R2:\s+0x([0-9A-Fa-f]+)`)
	regR3Regex := regexp.MustCompile(`R3:\s+0x([0-9A-Fa-f]+)`)

	for _, line := range lines {
		// Парсим статистику
		if matches := statsRegex.FindStringSubmatch(line); matches != nil {
			if val, err := strconv.Atoi(matches[1]); err == nil {
				result.InstructionsExecuted = val
			}
		}

		if matches := cyclesRegex.FindStringSubmatch(line); matches != nil {
			if val, err := strconv.Atoi(matches[1]); err == nil {
				result.Cycles = val
			}
		}

		// Парсим GPIO состояние
		if matches := gpioRegex.FindStringSubmatch(line); matches != nil {
			portName := matches[1]  // A, B, C...
			if val, err := strconv.ParseUint(matches[2], 16, 32); err == nil {
				result.GPIOState[fmt.Sprintf("PORT_%s_ODR", portName)] = uint32(val)
			}
		}

		// Парсим UART вывод
		if matches := uartRegex.FindStringSubmatch(line); matches != nil {
			if val, err := strconv.ParseUint(matches[1], 16, 16); err == nil {
				result.UARTOutput += string(byte(val))
			}
		}
		
		// Парсим значения регистров CPU
		if matches := regR0Regex.FindStringSubmatch(line); matches != nil {
			if val, err := strconv.ParseUint(matches[1], 16, 32); err == nil {
				result.GPIOState["R0"] = uint32(val)
			}
		}
		if matches := regR1Regex.FindStringSubmatch(line); matches != nil {
			if val, err := strconv.ParseUint(matches[1], 16, 32); err == nil {
				result.GPIOState["R1"] = uint32(val)
			}
		}
		if matches := regR2Regex.FindStringSubmatch(line); matches != nil {
			if val, err := strconv.ParseUint(matches[1], 16, 32); err == nil {
				result.GPIOState["R2"] = uint32(val)
			}
		}
		if matches := regR3Regex.FindStringSubmatch(line); matches != nil {
			if val, err := strconv.ParseUint(matches[1], 16, 32); err == nil {
				result.GPIOState["R3"] = uint32(val)
			}
		}
	}

	// Проверяем на ошибки в выводе
	if strings.Contains(output, "[ERROR]") {
		result.Status = "error"
		// Извлекаем сообщение об ошибке
		errorRegex := regexp.MustCompile(`\[ERROR\] (.+)`)
		if matches := errorRegex.FindStringSubmatch(output); matches != nil {
			result.ErrorMessage = matches[1]
		}
	}
}
