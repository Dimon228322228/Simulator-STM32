package keydb

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"time"

	"github.com/go-redis/redis/v8"
	"stm32-orchestrator/types"
)

// Client представляет клиент для подключения к KeyDB/Redis
type Client struct {
	redis *redis.Client
	ctx   context.Context
}

// Config представляет конфигурацию подключения
type Config struct {
	Addr     string
	Password string
	DB       int
}

// NewClient создаёт новый клиент KeyDB
func NewClient(cfg Config) *Client {
	rdb := redis.NewClient(&redis.Options{
		Addr:     cfg.Addr,
		Password: cfg.Password,
		DB:       cfg.DB,
	})

	ctx := context.Background()

	// Проверяем подключение
	if err := rdb.Ping(ctx).Err(); err != nil {
		log.Printf("[WARN] Cannot connect to KeyDB: %v", err)
		log.Printf("[INFO] Running without queue - will accept tasks via HTTP/file")
	}

	return &Client{
		redis: rdb,
		ctx:   ctx,
	}
}

// Close закрывает подключение к KeyDB
func (c *Client) Close() error {
	return c.redis.Close()
}

// PopTask извлекает задачу из очереди (блокирующая операция)
func (c *Client) PopTask(queueName string, timeout time.Duration) (*types.Task, error) {
	result, err := c.redis.BLPop(c.ctx, timeout, queueName).Result()
	if err != nil {
		if err == redis.Nil {
			return nil, nil // Таймаут, задач нет
		}
		return nil, fmt.Errorf("failed to pop task: %w", err)
	}

	if len(result) < 2 {
		return nil, fmt.Errorf("invalid response from Redis")
	}

	var task types.Task
	if err := json.Unmarshal([]byte(result[1]), &task); err != nil {
		return nil, fmt.Errorf("failed to unmarshal task: %w", err)
	}

	return &task, nil
}

// PushResult отправляет результат выполнения задачи обратно в KeyDB
func (c *Client) PushResult(resultQueueName string, result *types.TaskResult) error {
	data, err := json.Marshal(result)
	if err != nil {
		return fmt.Errorf("failed to marshal result: %w", err)
	}

	if err := c.redis.RPush(c.ctx, resultQueueName, data).Err(); err != nil {
		return fmt.Errorf("failed to push result: %w", err)
	}

	log.Printf("[INFO] Pushed result for task %s to %s", result.TaskID, resultQueueName)
	return nil
}

// PopTaskNoBlock извлекает задачу без блокировки (для тестов)
func (c *Client) PopTaskNoBlock(queueName string) (*types.Task, error) {
	result, err := c.redis.LPop(c.ctx, queueName).Result()
	if err != nil {
		if err == redis.Nil {
			return nil, nil // Очередь пуста
		}
		return nil, fmt.Errorf("failed to pop task: %w", err)
	}

	var task types.Task
	if err := json.Unmarshal([]byte(result), &task); err != nil {
		return nil, fmt.Errorf("failed to unmarshal task: %w", err)
	}

	return &task, nil
}
