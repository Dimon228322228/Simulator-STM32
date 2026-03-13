#include "execute.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Вспомогательная функция: извлечь биты из числа
static inline uint32_t get_bits(uint32_t value, int start, int end) {
    uint32_t mask = (1U << (end - start + 1)) - 1;
    return (value >> start) & mask;
}

// Вспомогательная функция: проверка знака и расширение
static inline int32_t sign_extend(uint32_t value, int bits) {
    uint32_t sign_bit = 1U << (bits - 1);
    if (value & sign_bit) {
        return value | (~0U << bits); // Заполняем единицами старшие биты
    }
    return value;
}

// Вспомогательная функция: получение операнда2 для арифметических инструкций
static inline uint32_t get_operand2(uint16_t instr, CPU_State *cpu) {
    // Проверяем формат операнда
    uint32_t op2_type = get_bits(instr, 11, 12);
    
    if (op2_type == 0b00) {
        // Непосредственный операнд (8 бит)
        return get_bits(instr, 0, 7);
    } else if (op2_type == 0b01) {
        // Регистр с сдвигом
        uint32_t rm = get_bits(instr, 0, 2);
        uint32_t shift_type = get_bits(instr, 3, 4);
        uint32_t shift_imm = get_bits(instr, 6, 10);
        
        // Простая реализация сдвигов
        switch(shift_type) {
            case 0b00: // LSL
                return cpu->regs[rm] << shift_imm;
            case 0b01: // LSR
                return cpu->regs[rm] >> shift_imm;
            case 0b10: // ASR
                if (cpu->regs[rm] & 0x80000000) {
                    // Отрицательное число - арифметический сдвиг
                    return (cpu->regs[rm] >> shift_imm) | (0xFFFFFFFF << (32 - shift_imm));
                } else {
                    return cpu->regs[rm] >> shift_imm;
                }
            case 0b11: // ROR
                return (cpu->regs[rm] >> shift_imm) | (cpu->regs[rm] << (32 - shift_imm));
        }
    } else if (op2_type == 0b10) {
        // Регистр (без сдвига)
        uint32_t rm = get_bits(instr, 0, 2);
        return cpu->regs[rm];
    }
    
    return 0;
}

// Вспомогательная функция: чтение регистра по индексу
static inline uint32_t get_register_value(uint16_t instr, int reg_index, CPU_State *cpu) {
    if (reg_index < 8) {
        return cpu->regs[reg_index];
    } else {
        // Для регистров R8-R12 используем специальные значения
        return cpu->regs[reg_index];
    }
}

void simulator_step(Simulator *sim) {
    CPU_State *cpu = &sim->cpu;
    Memory *mem = &sim->mem;

    // 1. FETCH: Считываем 16-битную инструкцию по текущему PC
    uint16_t instr = memory_read_halfword(mem, cpu->pc);
    
    // Увеличиваем PC (в Thumb режиме PC увеличивается на 2 после чтения)
    uint32_t current_pc = cpu->pc;
    cpu->pc += 2;

    // 2. DECODE & EXECUTE
    // Определяем тип инструкции по старшим битам
    
    // Для 16-битных инструкций определяем тип по маске
    uint16_t opcode = get_bits(instr, 11, 15);
    
    // Для 32-битных инструкций (в будущем)
    // uint32_t instr32 = memory_read_word(mem, cpu->pc);
    
    switch (opcode) {
        case 0b00100: { // MOV Rd, #imm8 (синтаксис: MOVS Rd, #imm8)
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            cpu->regs[rd] = imm8;
            printf("[EXEC] MOV R%u, #0x%02X -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b00011: { // Group: ADD/SUB register
            uint16_t sub_op = get_bits(instr, 9, 10);
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            
            if (sub_op == 0b00) { // ADD Rd, Rn, Rm
                cpu->regs[rd] = cpu->regs[rn] + cpu->regs[rm];
                printf("[EXEC] ADD R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            } else if (sub_op == 0b01) { // SUB Rd, Rn, Rm
                cpu->regs[rd] = cpu->regs[rn] - cpu->regs[rm];
                printf("[EXEC] SUB R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            } else {
                printf("[EXEC] Unknown ADD/SUB variant: 0x%04X\n", instr);
            }
            break;
        }

        case 0b11100: { // Unconditional Branch (B)
            int32_t offset = sign_extend(get_bits(instr, 0, 10), 11);
            // Смещение умножается на 2 (выравнивание по полуслову)
            uint32_t target = (current_pc + 4) + (offset << 1);
            // Выравнивание по границе слова (маскируем бит 1)
            target &= ~0x2U;

            printf("[EXEC] B 0x%08X (Offset: %d)\n", target, offset);
            cpu->pc = target;
            break;
        }

        // Новые инструкции
        case 0b00000: { // ADD Rd, Rn, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = cpu->regs[rn] + operand2;
            printf("[EXEC] ADD R%u, R%u, #0x%08X -> R%u = 0x%08X\n", rd, rn, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b00001: { // SUB Rd, Rn, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = cpu->regs[rn] - operand2;
            printf("[EXEC] SUB R%u, R%u, #0x%08X -> R%u = 0x%08X\n", rd, rn, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b00010: { // AND Rd, Rn, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = cpu->regs[rn] & operand2;
            printf("[EXEC] AND R%u, R%u, #0x%08X -> R%u = 0x%08X\n", rd, rn, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b00101: { // EOR Rd, Rn, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = cpu->regs[rn] ^ operand2;
            printf("[EXEC] EOR R%u, R%u, #0x%08X -> R%u = 0x%08X\n", rd, rn, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b00110: { // LSL Rd, Rm, #imm5
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rm = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            cpu->regs[rd] = cpu->regs[rm] << imm5;
            printf("[EXEC] LSL R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rm, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b00111: { // LSR Rd, Rm, #imm5
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rm = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            cpu->regs[rd] = cpu->regs[rm] >> imm5;
            printf("[EXEC] LSR R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rm, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b01000: { // ASR Rd, Rm, #imm5
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rm = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            if (cpu->regs[rm] & 0x80000000) {
                // Отрицательное число - арифметический сдвиг
                cpu->regs[rd] = (cpu->regs[rm] >> imm5) | (0xFFFFFFFF << (32 - imm5));
            } else {
                cpu->regs[rd] = cpu->regs[rm] >> imm5;
            }
            printf("[EXEC] ASR R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rm, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b01001: { // ADC Rd, Rn, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = cpu->regs[rn] + operand2 + (cpu->xpsr & 0x100 ? 1 : 0); // C-флаг
            printf("[EXEC] ADC R%u, R%u, #0x%08X -> R%u = 0x%08X\n", rd, rn, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b01010: { // SBC Rd, Rn, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = cpu->regs[rn] - operand2 - (cpu->xpsr & 0x100 ? 0 : 1); // C-флаг
            printf("[EXEC] SBC R%u, R%u, #0x%08X -> R%u = 0x%08X\n", rd, rn, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b01011: { // ROR Rd, Rm, #imm5
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rm = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            cpu->regs[rd] = (cpu->regs[rm] >> imm5) | (cpu->regs[rm] << (32 - imm5));
            printf("[EXEC] ROR R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rm, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b01100: { // TST Rd, Operand2
            uint32_t rd = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            uint32_t result = cpu->regs[rd] & operand2;
            // Обновляем флаги
            cpu->xpsr = (cpu->xpsr & ~0x80000000) | (result == 0 ? 0x40000000 : 0); // Z-флаг
            cpu->xpsr = (cpu->xpsr & ~0x80000000) | (result & 0x80000000 ? 0x80000000 : 0); // N-флаг
            printf("[EXEC] TST R%u, #0x%08X -> Result = 0x%08X\n", rd, operand2, result);
            break;
        }

        case 0b01101: { // NEG Rd, Rm
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rm = get_bits(instr, 3, 5);
            cpu->regs[rd] = ~cpu->regs[rm] + 1;
            printf("[EXEC] NEG R%u, R%u -> R%u = 0x%08X\n", rd, rm, rd, cpu->regs[rd]);
            break;
        }

        case 0b01110: { // CMP Rd, Operand2
            uint32_t rd = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            uint32_t result = cpu->regs[rd] - operand2;
            // Обновляем флаги
            cpu->xpsr = (cpu->xpsr & ~0x80000000) | (result == 0 ? 0x40000000 : 0); // Z-флаг
            cpu->xpsr = (cpu->xpsr & ~0x80000000) | (result & 0x80000000 ? 0x80000000 : 0); // N-флаг
            cpu->xpsr = (cpu->xpsr & ~0x100) | (result > cpu->regs[rd] ? 0x100 : 0); // C-флаг
            printf("[EXEC] CMP R%u, #0x%08X -> Result = 0x%08X\n", rd, operand2, result);
            break;
        }

        case 0b01111: { // CMN Rd, Operand2
            uint32_t rd = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            uint32_t result = cpu->regs[rd] + operand2;
            // Обновляем флаги
            cpu->xpsr = (cpu->xpsr & ~0x80000000) | (result == 0 ? 0x40000000 : 0); // Z-флаг
            cpu->xpsr = (cpu->xpsr & ~0x80000000) | (result & 0x80000000 ? 0x80000000 : 0); // N-флаг
            printf("[EXEC] CMN R%u, #0x%08X -> Result = 0x%08X\n", rd, operand2, result);
            break;
        }

        case 0b10000: { // ORR Rd, Rn, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = cpu->regs[rn] | operand2;
            printf("[EXEC] ORR R%u, R%u, #0x%08X -> R%u = 0x%08X\n", rd, rn, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b10001: { // MUL Rd, Rn, Rm
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rm = get_bits(instr, 0, 2);
            cpu->regs[rd] = cpu->regs[rn] * cpu->regs[rm];
            printf("[EXEC] MUL R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            break;
        }

        case 0b10010: { // BIC Rd, Rn, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = cpu->regs[rn] & ~operand2;
            printf("[EXEC] BIC R%u, R%u, #0x%08X -> R%u = 0x%08X\n", rd, rn, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b10011: { // MVN Rd, Operand2
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t operand2 = get_operand2(instr, cpu);
            cpu->regs[rd] = ~operand2;
            printf("[EXEC] MVN R%u, #0x%08X -> R%u = 0x%08X\n", rd, operand2, rd, cpu->regs[rd]);
            break;
        }

        case 0b10100: { // ADDW Rd, Rn, #imm12
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm12 = get_bits(instr, 0, 11);
            cpu->regs[rd] = cpu->regs[rn] + imm12;
            printf("[EXEC] ADDW R%u, R%u, #0x%03X -> R%u = 0x%08X\n", rd, rn, imm12, rd, cpu->regs[rd]);
            break;
        }

        case 0b10101: { // SUBW Rd, Rn, #imm12
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm12 = get_bits(instr, 0, 11);
            cpu->regs[rd] = cpu->regs[rn] - imm12;
            printf("[EXEC] SUBW R%u, R%u, #0x%03X -> R%u = 0x%08X\n", rd, rn, imm12, rd, cpu->regs[rd]);
            break;
        }

        case 0b10110: { // MOVW Rd, #imm16
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm16 = get_bits(instr, 0, 11) | (get_bits(instr, 12, 15) << 12);
            cpu->regs[rd] = imm16;
            printf("[EXEC] MOVW R%u, #0x%04X -> R%u = 0x%08X\n", rd, imm16, rd, cpu->regs[rd]);
            break;
        }

        case 0b10111: { // MOVT Rd, #imm16
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm16 = get_bits(instr, 0, 11) | (get_bits(instr, 12, 15) << 12);
            cpu->regs[rd] = (cpu->regs[rd] & 0x0000FFFF) | (imm16 << 16);
            printf("[EXEC] MOVT R%u, #0x%04X -> R%u = 0x%08X\n", rd, imm16, rd, cpu->regs[rd]);
            break;
        }

        case 0b11000: { // LDR Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 0, 2) | (get_bits(instr, 6, 10) << 3);
            uint32_t addr = cpu->regs[rn] + imm5;
            cpu->regs[rd] = memory_read_word(mem, addr);
            printf("[EXEC] LDR R%u, [%u, #0x%02X] -> R%u = 0x%08X\n", rd, rn, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b11001: { // STR Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 0, 2) | (get_bits(instr, 6, 10) << 3);
            uint32_t addr = cpu->regs[rn] + imm5;
            memory_write_byte(mem, addr, cpu->regs[rd] & 0xFF);
            memory_write_byte(mem, addr + 1, (cpu->regs[rd] >> 8) & 0xFF);
            memory_write_byte(mem, addr + 2, (cpu->regs[rd] >> 16) & 0xFF);
            memory_write_byte(mem, addr + 3, (cpu->regs[rd] >> 24) & 0xFF);
            printf("[EXEC] STR R%u, [%u, #0x%02X]\n", rd, rn, imm5);
            break;
        }

        case 0b11010: { // LDRB Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 0, 2) | (get_bits(instr, 6, 10) << 3);
            uint32_t addr = cpu->regs[rn] + imm5;
            cpu->regs[rd] = memory_read_byte(mem, addr);
            printf("[EXEC] LDRB R%u, [%u, #0x%02X] -> R%u = 0x%02X\n", rd, rn, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b11011: { // STRB Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 0, 2) | (get_bits(instr, 6, 10) << 3);
            uint32_t addr = cpu->regs[rn] + imm5;
            memory_write_byte(mem, addr, cpu->regs[rd] & 0xFF);
            printf("[EXEC] STRB R%u, [%u, #0x%02X]\n", rd, rn, imm5);
            break;
        }

        case 0b11101: { // LDRH Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 0, 2) | (get_bits(instr, 6, 10) << 3);
            uint32_t addr = cpu->regs[rn] + imm5;
            cpu->regs[rd] = memory_read_halfword(mem, addr);
            printf("[EXEC] LDRH R%u, [%u, #0x%02X] -> R%u = 0x%04X\n", rd, rn, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b11110: { // STRH Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 0, 2) | (get_bits(instr, 6, 10) << 3);
            uint32_t addr = cpu->regs[rn] + imm5;
            memory_write_byte(mem, addr, cpu->regs[rd] & 0xFF);
            memory_write_byte(mem, addr + 1, (cpu->regs[rd] >> 8) & 0xFF);
            printf("[EXEC] STRH R%u, [%u, #0x%02X]\n", rd, rn, imm5);
            break;
        }

        case 0b11111: { // BX Rd
            uint32_t rd = get_bits(instr, 3, 5);
            uint32_t target = cpu->regs[rd];
            // Убираем бит 0 для Thumb режима
            cpu->pc = target & ~0x1;
            printf("[EXEC] BX R%u -> PC = 0x%08X\n", rd, cpu->pc);
            break;
        }

        default:
            printf("[EXEC] Unknown Instruction: 0x%04X at PC 0x%08X\n", instr, current_pc);
            // Для безопасности остановим симуляцию при встрече неизвестной инструкции
            cpu->pc = 0xFFFFFFFF; // Невероятный адрес для останова
            break;
    }
}

void simulator_run(Simulator *sim) {
    while (sim->cpu.pc < FLASH_BASE_ADDR + FLASH_SIZE) {
        // Простая защита от зацикливания
        static int max_steps = 20;
        if (max_steps-- <= 0) {
            printf("[SIM] Step limit reached. Stopping.\n");
            break;
        }
        simulator_step(sim);
    }
}