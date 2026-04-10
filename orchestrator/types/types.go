package types

// Task представляет задание на симуляцию
type Task struct {
	TaskID     string     `json:"task_id"`
	StudentID  string     `json:"student_id"`
	LabNumber  int        `json:"lab_number"`
	Binary     string     `json:"binary"` // Base64 encoded .bin file
	TimeoutSec int        `json:"timeout_sec"`
	Config     TaskConfig `json:"config"`
}

// TaskConfig представляет конфигурацию задачи
type TaskConfig struct {
	GPIOInput string `json:"gpio_input,omitempty"`
	UARTInput string `json:"uart_input,omitempty"`
}

// TaskResult представляет результат выполнения задачи
type TaskResult struct {
	TaskID               string            `json:"task_id"`
	Status               string            `json:"status"` // "success", "error", "timeout"
	UARTOutput           string            `json:"uart_output,omitempty"`
	GPIOState            map[string]uint32 `json:"gpio_state,omitempty"`
	InstructionsExecuted int               `json:"instructions_executed,omitempty"`
	Cycles               int               `json:"cycles,omitempty"`
	ErrorMessage         string            `json:"error_message,omitempty"`
	Stdout               string            `json:"stdout,omitempty"`
}
