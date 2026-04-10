package main

import (
	"context"
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"

	"stm32-orchestrator/keydb"
	"stm32-orchestrator/simulator"
	"stm32-orchestrator/types"
)

// Config представляет конфигурацию приложения
type Config struct {
	RedisAddr      string
	RedisPassword  string
	TaskQueue      string
	ResultQueue    string
	SimulatorPath  string
	DefaultTimeout time.Duration
	PollInterval   time.Duration
	Mode           string // "queue" или "file"
}

func main() {
	// Парсинг флагов командной строки
	config := parseFlags()

	log.Printf("[INFO] Starting STM32 Orchestrator")
	log.Printf("[INFO] Configuration:")
	log.Printf("[INFO]   Redis: %s", config.RedisAddr)
	log.Printf("[INFO]   Task Queue: %s", config.TaskQueue)
	log.Printf("[INFO]   Result Queue: %s", config.ResultQueue)
	log.Printf("[INFO]   Simulator: %s", config.SimulatorPath)
	log.Printf("[INFO]   Mode: %s", config.Mode)

	// Создаём контекст с обработкой сигналов
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Обработка сигналов завершения
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigChan
		log.Printf("[INFO] Received signal %v, shutting down...", sig)
		cancel()
	}()

	// Создаём клиент KeyDB
	keydbClient := keydb.NewClient(keydb.Config{
		Addr:     config.RedisAddr,
		Password: config.RedisPassword,
		DB:       0,
	})
	defer keydbClient.Close()

	// Создаём runner для симулятора
	simRunner := simulator.NewRunner(config.SimulatorPath, config.DefaultTimeout)

	// Запускаем основной цикл в зависимости от режима
	if config.Mode == "queue" {
		runQueueMode(ctx, keydbClient, simRunner, config)
	} else if config.Mode == "file" {
		runFileMode(ctx, keydbClient, simRunner, config)
	} else {
		log.Fatalf("[ERROR] Unknown mode: %s", config.Mode)
	}

	log.Printf("[INFO] Orchestrator stopped")
}

func parseFlags() Config {
	config := Config{}

	flag.StringVar(&config.RedisAddr, "redis", "localhost:6379", "Redis/KeyDB address")
	flag.StringVar(&config.RedisPassword, "redis-pass", "", "Redis password")
	flag.StringVar(&config.TaskQueue, "task-queue", "simulator:tasks", "Task queue name")
	flag.StringVar(&config.ResultQueue, "result-queue", "simulator:results", "Result queue name")
	flag.StringVar(&config.SimulatorPath, "simulator", "./stm32_sim", "Path to simulator binary")
	flag.DurationVar(&config.DefaultTimeout, "timeout", 5*time.Second, "Default execution timeout")
	flag.DurationVar(&config.PollInterval, "poll-interval", 1*time.Second, "Queue poll interval")
	flag.StringVar(&config.Mode, "mode", "queue", "Operation mode: queue or file")

	// Для режима file
	inputFile := flag.String("file", "", "Input task file (for file mode)")
	outputFile := flag.String("output", "", "Output result file (for file mode)")

	flag.Parse()

	// Сохраняем пути файлов в переменные окружения для file mode
	if *inputFile != "" {
		os.Setenv("INPUT_FILE", *inputFile)
	}
	if *outputFile != "" {
		os.Setenv("OUTPUT_FILE", *outputFile)
	}

	return config
}

// runQueueMode работает в режиме очереди (KeyDB)
func runQueueMode(ctx context.Context, client *keydb.Client, runner *simulator.Runner, config Config) {
	log.Printf("[INFO] Running in queue mode")

	for {
		select {
		case <-ctx.Done():
			return
		default:
		}

		// Извлекаем задачу из очереди
		task, err := client.PopTask(config.TaskQueue, config.PollInterval)
		if err != nil {
			log.Printf("[ERROR] Failed to pop task: %v", err)
			time.Sleep(config.PollInterval)
			continue
		}

		if task == nil {
			// Очередь пуста, ждём
			continue
		}

		log.Printf("[INFO] Received task %s (student: %s, lab: %d)",
			task.TaskID, task.StudentID, task.LabNumber)

		// Выполняем задачу
		result, err := runner.RunTask(ctx, task)
		if err != nil {
			log.Printf("[ERROR] Failed to run task %s: %v", task.TaskID, err)
			result = &types.TaskResult{
				TaskID:       task.TaskID,
				Status:       "error",
				ErrorMessage: err.Error(),
			}
		}

		// Отправляем результат
		if err := client.PushResult(config.ResultQueue, result); err != nil {
			log.Printf("[ERROR] Failed to push result: %v", err)
		}
	}
}

// runFileMode работает в режиме файла (для тестов)
func runFileMode(ctx context.Context, client *keydb.Client, runner *simulator.Runner, config Config) {
	log.Printf("[INFO] Running in file mode")

	inputFile := os.Getenv("INPUT_FILE")
	outputFile := os.Getenv("OUTPUT_FILE")

	if inputFile == "" {
		log.Fatalf("[ERROR] INPUT_FILE not specified")
	}

	// Читаем задачу из файла
	data, err := os.ReadFile(inputFile)
	if err != nil {
		log.Fatalf("[ERROR] Failed to read input file: %v", err)
	}

	var task types.Task
	if err := json.Unmarshal(data, &task); err != nil {
		log.Fatalf("[ERROR] Failed to parse task: %v", err)
	}

	log.Printf("[INFO] Loaded task %s from %s", task.TaskID, inputFile)

	// Если бинарник не в base64, а в отдельном файле
	if task.Binary == "" {
		binFile := os.Getenv("BINARY_FILE")
		if binFile != "" {
			binData, err := os.ReadFile(binFile)
			if err != nil {
				log.Fatalf("[ERROR] Failed to read binary file: %v", err)
			}
			task.Binary = base64.StdEncoding.EncodeToString(binData)
			log.Printf("[INFO] Loaded binary from %s (%d bytes)", binFile, len(binData))
		}
	}

	// Выполняем задачу
	result, err := runner.RunTask(ctx, &task)
	if err != nil {
		log.Fatalf("[ERROR] Failed to run task: %v", err)
	}

	// Записываем результат в файл
	resultData, err := json.MarshalIndent(result, "", "  ")
	if err != nil {
		log.Fatalf("[ERROR] Failed to marshal result: %v", err)
	}

	if outputFile != "" {
		if err := os.WriteFile(outputFile, resultData, 0644); err != nil {
			log.Fatalf("[ERROR] Failed to write output file: %v", err)
		}
		log.Printf("[INFO] Result written to %s", outputFile)
	} else {
		fmt.Println(string(resultData))
	}
}
