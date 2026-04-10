#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "cpu_state.h"
#include "execute.h"

// Глобальная статистика выполнения
typedef struct {
    uint32_t instructions_executed;
    uint32_t cycles;
    uint32_t uart_bytes_sent;
} SimulatorStats;

static SimulatorStats g_stats = {0, 0, 0};

// Функция загрузки бинарного файла во Flash
int load_binary_file(Memory *mem, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "[ERROR] Cannot open file: %s\n", filename);
        return 0;
    }

    // Определяем размер файла
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Проверяем, что файл помещается во Flash
    if (file_size > FLASH_SIZE) {
        fprintf(stderr, "[ERROR] File too large (%ld bytes, max %d bytes)\n", 
                file_size, FLASH_SIZE);
        fclose(file);
        return 0;
    }

    // Читаем файл во Flash память
    size_t bytes_read = fread(mem->flash, 1, file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "[ERROR] Failed to read entire file\n");
        return 0;
    }

    printf("[INIT] Loaded %ld bytes from %s into Flash at 0x%08X\n", 
           file_size, filename, FLASH_BASE_ADDR);
    return 1;
}

// Функция вывода статистики
void print_statistics(SimulatorStats *stats) {
    printf("\n");
    printf("========================================\n");
    printf("       SIMULATION STATISTICS\n");
    printf("========================================\n");
    printf("[STATS] Instructions executed: %u\n", stats->instructions_executed);
    printf("[STATS] CPU cycles: %u\n", stats->cycles);
    printf("[STATS] UART bytes sent: %u\n", stats->uart_bytes_sent);
    printf("========================================\n");
}

// Функция вывода состояния регистров
void print_cpu_state(CPU_State *cpu) {
    printf("\n[CPU] Final register state:\n");
    printf("  R0:  0x%08X    R4:  0x%08X    R8:  0x%08X   R12: 0x%08X\n",
           cpu->regs[0], cpu->regs[4], cpu->regs[8], cpu->regs[12]);
    printf("  R1:  0x%08X    R5:  0x%08X    R9:  0x%08X   R13: 0x%08X (SP)\n",
           cpu->regs[1], cpu->regs[5], cpu->regs[9], cpu->regs[13]);
    printf("  R2:  0x%08X    R6:  0x%08X    R10: 0x%08X   R14: 0x%08X (LR)\n",
           cpu->regs[2], cpu->regs[6], cpu->regs[10], cpu->regs[14]);
    printf("  R3:  0x%08X    R7:  0x%08X    R11: 0x%08X   R15: 0x%08X (PC)\n",
           cpu->regs[3], cpu->regs[7], cpu->regs[11], cpu->regs[15]);
    printf("  xPSR: 0x%08X\n", cpu->xpsr);
}

void print_usage(const char *program_name) {
    printf("STM32F103C8T6 Simulator\n");
    printf("=======================\n");
    printf("Usage: %s [options] [firmware.bin]\n", program_name);
    printf("\nOptions:\n");
    printf("  --help, -h     Show this help message\n");
    printf("  --demo         Run built-in demo program (no file required)\n");
    printf("  --max-steps N  Limit execution to N instructions (default: 100)\n");
    printf("\nExamples:\n");
    printf("  %s my_program.bin\n", program_name);
    printf("  %s --demo --max-steps 100\n", program_name);
}

int main(int argc, char *argv[]) {
    Simulator sim;
    SimulatorStats stats = {0, 0, 0};
    
    const char *firmware_file = NULL;
    int run_demo = 1;  // По умолчанию запускаем демо
    int max_steps = 100;
    
    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--demo") == 0) {
            run_demo = 1;
        }
        else if (strcmp(argv[i], "--max-steps") == 0 && i + 1 < argc) {
            max_steps = atoi(argv[++i]);
        }
        else if (argv[i][0] != '-') {
            firmware_file = argv[i];
            run_demo = 0;  // Если указан файл, не запускаем демо
        }
        else {
            fprintf(stderr, "[ERROR] Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    printf("Initializing Simulator...\n");
    printf("==========================\n\n");

    // Инициализация памяти
    if (!memory_init(&sim.mem)) {
        fprintf(stderr, "[ERROR] Failed to initialize memory\n");
        return 1;
    }

    // Инициализация периферии
    gpio_init(&sim.gpio);
    tim6_init(&sim.tim6);
    nvic_init(&sim.nvic);
    bus_matrix_init(&sim.bus);
    rcc_init(&sim.rcc);
    dma_init(&sim.dma);

    // Инициализация коммуникационных интерфейсов
    usart_init(&sim.usart1, USART1_BASE_ADDR);
    usart_init(&sim.usart2, USART2_BASE_ADDR);
    usart_init(&sim.usart3, USART3_BASE_ADDR);
    spi_init(&sim.spi1, SPI1_BASE_ADDR);
    spi_init(&sim.spi2, SPI2_BASE_ADDR);
    spi_init(&sim.spi3, SPI3_BASE_ADDR);
    i2c_init(&sim.i2c1, I2C1_BASE_ADDR);
    i2c_init(&sim.i2c2, I2C2_BASE_ADDR);

    // Инициализация CPU
    cpu_reset(&sim.cpu);

    // Устанавливаем начальный PC
    sim.cpu.pc = FLASH_BASE_ADDR;

    // Загружаем прошивку
    if (firmware_file) {
        if (!load_binary_file(&sim.mem, firmware_file)) {
            memory_free(&sim.mem);
            return 1;
        }
    }
    else if (run_demo) {
        /* Demo program: add two numbers and loop forever */
        uint16_t program[] = {
            0x2005,  /* MOV  R0, #5       */
            0x2103,  /* MOV  R1, #3       */
            0x1842,  /* ADDS R2, R0, R1   → R2 = 8 */
            0xE7FF   /* B    .            (infinite loop) */
        };
        
        memcpy(sim.mem.flash, program, sizeof(program));
        printf("[INIT] Loaded demo program (%zu bytes)\n", sizeof(program));
    }

    printf("\nStarting simulation...\n");
    printf("----------------------\n\n");

    /* Run simulation with step limit */
    simulator_run(&sim, max_steps);

    /* Collect statistics */
    stats.instructions_executed = max_steps;  /* approximate */
    stats.cycles = max_steps;

    /* Print results */
    printf("\n==========================\n");
    printf("Simulation finished.\n");
    
    print_cpu_state(&sim.cpu);
    print_statistics(&stats);

    memory_free(&sim.mem);
    return 0;
}
