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
        uint32_t shift_imm_full = get_bits(instr, 6, 10);
        // Ограничиваем количество битов для сдвига, чтобы избежать неопределенного поведения
        uint32_t shift_imm = shift_imm_full & 0x1F; // Маска для 5 битов (0-31)
        
        // Простая реализация сдвигов
        switch(shift_type) {
            case 0b00: // LSL
                return cpu->regs[rm] << shift_imm;
            case 0b01: // LSR
                return cpu->regs[rm] >> shift_imm;
            case 0b10: // ASR
                if (cpu->regs[rm] & 0x80000000) {
                    // Отрицательное число - арифметический сдвиг
                    if (shift_imm == 0) {
                        return cpu->regs[rm];
                    } else {
                        return (cpu->regs[rm] >> shift_imm) | (0xFFFFFFFFU << (32 - shift_imm));
                    }
                } else {
                    return cpu->regs[rm] >> shift_imm;
                }
            case 0b11: // ROR
                if (shift_imm == 0) {
                    return cpu->regs[rm]; // Если сдвиг равен 0, возвращаем исходное значение
                } else {
                    return (cpu->regs[rm] >> shift_imm) | (cpu->regs[rm] << (32 - shift_imm));
                }
        }
    } else if (op2_type == 0b10) {
        // Регистр (без сдвига)
        uint32_t rm = get_bits(instr, 0, 2);
        return cpu->regs[rm];
    }
    
    return 0;
}

// Вспомогательная функция: обновление флагов N, Z, C, V
void update_flags(CPU_State *cpu, uint32_t result, uint32_t carry_in) {
    // N флаг (Negative) - устанавливается, если старший бит результата равен 1
    cpu->xpsr = (cpu->xpsr & ~0x80000000) | (result & 0x80000000 ? 0x80000000 : 0);

    // Z флаг (Zero) - устанавливается, если результат равен 0
    cpu->xpsr = (cpu->xpsr & ~0x40000000) | (result == 0 ? 0x40000000 : 0);

    // C флаг (Carry) - устанавливается при переносе
    // Для операций с переносом (ADC, SBC) этот флаг уже обновляется отдельно
}

// Вспомогательная функция: обновление флагов для арифметических операций
void update_arithmetic_flags(CPU_State *cpu, uint32_t a, uint32_t b, uint32_t result, uint8_t is_subtract) {
    // N флаг (Negative) - устанавливается, если старший бит результата равен 1
    cpu->xpsr = (cpu->xpsr & ~0x80000000) | (result & 0x80000000 ? 0x80000000 : 0);
    
    // Z флаг (Zero) - устанавливается, если результат равен 0
    cpu->xpsr = (cpu->xpsr & ~0x40000000) | (result == 0 ? 0x40000000 : 0);
    
    // C флаг (Carry) - устанавливается при переносе
    if (is_subtract) {
        cpu->xpsr = (cpu->xpsr & ~0x100) | (a < b ? 0x100 : 0);
    } else {
        cpu->xpsr = (cpu->xpsr & ~0x100) | (result < a ? 0x100 : 0);
    }
    
    // V флаг (Overflow) - устанавливается при переполнении
    // Для signed arithmetic: V = (A_sign XOR B_sign) AND (A_sign XOR Result_sign)
    uint8_t a_sign = (a & 0x80000000) ? 1 : 0;
    uint8_t b_sign = (b & 0x80000000) ? 1 : 0;
    uint8_t result_sign = (result & 0x80000000) ? 1 : 0;
    cpu->xpsr = (cpu->xpsr & ~0x200) | ((a_sign ^ b_sign) & (a_sign ^ result_sign) ? 0x200 : 0);
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

// Вспомогательная функция: проверка условия выполнения инструкции
uint8_t check_condition(uint16_t instr, CPU_State *cpu) {
    // Получаем условный код (последние 4 бита для 16-битных инструкций)
    uint8_t cond = get_bits(instr, 0, 3);
    
    // Проверяем условие
    switch(cond) {
        case 0b0000: // EQ - Equal
            return (cpu->xpsr & 0x40000000) ? 1 : 0;
        case 0b0001: // NE - Not Equal
            return !(cpu->xpsr & 0x40000000);
        case 0b0010: // CS - Carry Set
            return (cpu->xpsr & 0x100) ? 1 : 0;
        case 0b0011: // CC - Carry Clear
            return !(cpu->xpsr & 0x100);
        case 0b0100: // MI - Minus
            return (cpu->xpsr & 0x80000000) ? 1 : 0;
        case 0b0101: // PL - Plus
            return !(cpu->xpsr & 0x80000000);
        case 0b0110: // VS - Overflow Set
            return (cpu->xpsr & 0x200) ? 1 : 0;
        case 0b0111: // VC - Overflow Clear
            return !(cpu->xpsr & 0x200);
        case 0b1000: // HI - Higher
            return ((cpu->xpsr & 0x100) && !(cpu->xpsr & 0x40000000)) ? 1 : 0;
        case 0b1001: // LS - Lower or Same
            return (!(cpu->xpsr & 0x100) || (cpu->xpsr & 0x40000000)) ? 1 : 0;
        case 0b1010: // GE - Greater or Equal
            return ((cpu->xpsr & 0x80000000) == (cpu->xpsr & 0x200)) ? 1 : 0;
        case 0b1011: // LT - Less Than
            return ((cpu->xpsr & 0x80000000) != (cpu->xpsr & 0x200)) ? 1 : 0;
        case 0b1100: // GT - Greater Than
            return (!(cpu->xpsr & 0x40000000) && ((cpu->xpsr & 0x80000000) == (cpu->xpsr & 0x200))) ? 1 : 0;
        case 0b1101: // LE - Less or Equal
            return (cpu->xpsr & 0x40000000) || ((cpu->xpsr & 0x80000000) != (cpu->xpsr & 0x200)) ? 1 : 0;
        case 0b1110: // AL - Always (default)
            return 1;
        case 0b1111: // NV - Never (не используется в Thumb)
            return 0;
        default:
            return 1; // По умолчанию всегда выполняется
    }
}

// Обработчик арифметических инструкций
static void handle_arithmetic(uint16_t instr, uint16_t opcode, CPU_State *cpu, Memory *mem) {
    switch(opcode) {
        case 0b00000: { // ADD Rd, Rn, #imm3
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm3 = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] + imm3;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], imm3, result, 0);
            printf("[EXEC] ADD R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rn, imm3, rd, cpu->regs[rd]);
            break;
        }
        case 0b00001: { // SUB Rd, Rn, #imm3
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm3 = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] - imm3;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], imm3, result, 1);
            printf("[EXEC] SUB R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rn, imm3, rd, cpu->regs[rd]);
            break;
        }
        case 0b00010: { // ADD Rd, Rn, Rm
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] + cpu->regs[rm];
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 0);
            printf("[EXEC] ADD R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            break;
        }
        case 0b00011: { // SUB Rd, Rn, Rm
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] - cpu->regs[rm];
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 1);
            printf("[EXEC] SUB R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            break;
        }
        // Handle other arithmetic instructions as needed...
        default:
            break;
    }
}

// Обработчик load/store инструкций
static void handle_load_store(uint16_t instr, uint16_t opcode, CPU_State *cpu, Memory *mem) {
    switch(opcode) {
        case 0b01010: { // STR Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [R%u, R%u]\n", rd, rn, rm);
            break;
        }
        case 0b01011: { // STRH Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            if (!memory_write_halfword_safe(mem, addr, cpu->regs[rd] & 0xFFFF)) {
                printf("[ERROR] Invalid halfword memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STRH R%u, [R%u, R%u]\n", rd, rn, rm);
            break;
        }
        case 0b10100: { // STR Rd, [SP, #imm8]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t addr = cpu->regs[13] + (imm8 << 2);
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [SP, #%u]\n", rd, imm8);
            break;
        }
        case 0b11000: { // STR Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + (imm5 << 2);
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [R%u, #%u]\n", rd, rn, imm5);
            break;
        }
        case 0b11001: { // LDR Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + (imm5 << 2);
            // Проверяем, что адрес в допустимом диапазоне для чтения
            if ((addr >= FLASH_BASE_ADDR && addr + 3 < (FLASH_BASE_ADDR + FLASH_SIZE)) ||
                (addr >= SRAM_BASE_ADDR && addr + 3 < (SRAM_BASE_ADDR + SRAM_SIZE))) {
                cpu->regs[rd] = memory_read_word(mem, addr);
            } else {
                printf("[ERROR] Invalid memory read at address 0x%08X\n", addr);
                cpu->regs[rd] = 0xFFFFFFFF; // Значение ошибки
            }
            printf("[EXEC] LDR R%u, [R%u, #%u] -> R%u = 0x%08X\n", rd, rn, imm5, rd, cpu->regs[rd]);
            break;
        }
        // Add other load/store handlers here...
        default:
            break;
    }
}

// Обработчик инструкций ветвления
static void handle_branch(uint16_t instr, uint16_t opcode, CPU_State *cpu, Memory *mem) {
    switch(opcode) {
        case 0b11010: { // Conditional branch (B cond)
            int32_t offset = sign_extend(get_bits(instr, 0, 7), 8);
            uint32_t target = cpu->pc + (offset << 1);
            cpu->pc = target;
            printf("[EXEC] B.cond (Offset: %d)\n", offset);
            break;
        }
        case 0b11100: { // Unconditional Branch (B)
            int32_t offset = sign_extend(get_bits(instr, 0, 10), 11);
            uint32_t target = cpu->pc + (offset << 1);
            printf("[EXEC] B 0x%08X (Offset: %d)\n", target, offset);
            cpu->pc = target;
            break;
        }
        case 0b11111: { // BX Rd
            uint32_t rd = get_bits(instr, 3, 5);
            uint32_t target = cpu->regs[rd];
            cpu->pc = target & ~0x1U;
            printf("[EXEC] BX R%u -> PC = 0x%08X\n", rd, cpu->pc);
            break;
        }
        default:
            break;
    }
}

// Обработчик прочих инструкций
static void handle_misc(uint16_t instr, uint16_t opcode, CPU_State *cpu, Memory *mem) {
    switch(opcode) {
        case 0b00100: { // MOV Rd, #imm8 (синтаксис: MOVS Rd, #imm8)
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            cpu->regs[rd] = imm8;
            // Обновляем флаги для MOVS
            update_flags(cpu, imm8, 0);
            printf("[EXEC] MOV R%u, #0x%02X -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }
        case 0b10000: { // STRB Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + imm5;
            memory_write_byte(mem, addr, cpu->regs[rd] & 0xFF);
            printf("[EXEC] STRB R%u, [R%u, #%u]\n", rd, rn, imm5);
            break;
        }
        // Add other misc handlers here...
        default:
            printf("[EXEC] Unknown Instruction: 0x%04X at PC 0x%08X\n", instr, current_pc);
            // For safety, stop simulation when encountering an unknown instruction
            cpu->pc = 0xFFFFFFFF; // Impossible address to stop
            break;
    }
}

// Обработчик арифметических инструкций
static void handle_arithmetic(uint16_t instr, uint16_t opcode, CPU_State *cpu, Memory *mem) {
    switch(opcode) {
        case 0b00000: { // ADD Rd, Rn, #imm3
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm3 = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] + imm3;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], imm3, result, 0);
            printf("[EXEC] ADD R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rn, imm3, rd, cpu->regs[rd]);
            break;
        }
        case 0b00001: { // SUB Rd, Rn, #imm3
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm3 = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] - imm3;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], imm3, result, 1);
            printf("[EXEC] SUB R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rn, imm3, rd, cpu->regs[rd]);
            break;
        }
        case 0b00010: { // ADD Rd, Rn, Rm
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] + cpu->regs[rm];
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 0);
            printf("[EXEC] ADD R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            break;
        }
        case 0b00011: { // SUB Rd, Rn, Rm
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] - cpu->regs[rm];
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 1);
            printf("[EXEC] SUB R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            break;
        }
        // Handle other arithmetic instructions as needed...
        default:
            break;
    }
}

// Обработчик load/store инструкций
static void handle_load_store(uint16_t instr, uint16_t opcode, CPU_State *cpu, Memory *mem) {
    switch(opcode) {
        case 0b01010: { // STR Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [R%u, R%u]\n", rd, rn, rm);
            break;
        }
        case 0b01011: { // STRH Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            if (!memory_write_halfword_safe(mem, addr, cpu->regs[rd] & 0xFFFF)) {
                printf("[ERROR] Invalid halfword memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STRH R%u, [R%u, R%u]\n", rd, rn, rm);
            break;
        }
        case 0b10100: { // STR Rd, [SP, #imm8]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t addr = cpu->regs[13] + (imm8 << 2);
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [SP, #%u]\n", rd, imm8);
            break;
        }
        case 0b11000: { // STR Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + (imm5 << 2);
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [R%u, #%u]\n", rd, rn, imm5);
            break;
        }
        case 0b11001: { // LDR Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + (imm5 << 2);
            // Проверяем, что адрес в допустимом диапазоне для чтения
            if ((addr >= FLASH_BASE_ADDR && addr + 3 < (FLASH_BASE_ADDR + FLASH_SIZE)) ||
                (addr >= SRAM_BASE_ADDR && addr + 3 < (SRAM_BASE_ADDR + SRAM_SIZE))) {
                cpu->regs[rd] = memory_read_word(mem, addr);
            } else {
                printf("[ERROR] Invalid memory read at address 0x%08X\n", addr);
                cpu->regs[rd] = 0xFFFFFFFF; // Значение ошибки
            }
            printf("[EXEC] LDR R%u, [R%u, #%u] -> R%u = 0x%08X\n", rd, rn, imm5, rd, cpu->regs[rd]);
            break;
        }
        // Add other load/store handlers here...
        default:
            break;
    }
}

// Обработчик инструкций ветвления
static void handle_branch(uint16_t instr, uint16_t opcode, CPU_State *cpu, Memory *mem) {
    switch(opcode) {
        case 0b11010: { // Conditional branch (B cond)
            int32_t offset = sign_extend(get_bits(instr, 0, 7), 8);
            uint32_t target = cpu->pc + (offset << 1);
            cpu->pc = target;
            printf("[EXEC] B.cond (Offset: %d)\n", offset);
            break;
        }
        case 0b11100: { // Unconditional Branch (B)
            int32_t offset = sign_extend(get_bits(instr, 0, 10), 11);
            uint32_t target = cpu->pc + (offset << 1);
            printf("[EXEC] B 0x%08X (Offset: %d)\n", target, offset);
            cpu->pc = target;
            break;
        }
        case 0b11111: { // BX Rd
            uint32_t rd = get_bits(instr, 3, 5);
            uint32_t target = cpu->regs[rd];
            cpu->pc = target & ~0x1U;
            printf("[EXEC] BX R%u -> PC = 0x%08X\n", rd, cpu->pc);
            break;
        }
        default:
            break;
    }
}

// Обработчик прочих инструкций
static void handle_misc(uint16_t instr, uint16_t opcode, CPU_State *cpu, Memory *mem) {
    switch(opcode) {
        case 0b00100: { // MOV Rd, #imm8 (синтаксис: MOVS Rd, #imm8)
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            cpu->regs[rd] = imm8;
            // Обновляем флаги для MOVS
            update_flags(cpu, imm8, 0);
            printf("[EXEC] MOV R%u, #0x%02X -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }
        case 0b10000: { // STRB Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + imm5;
            memory_write_byte(mem, addr, cpu->regs[rd] & 0xFF);
            printf("[EXEC] STRB R%u, [R%u, #%u]\n", rd, rn, imm5);
            break;
        }
        // Add other misc handlers here...
        default:
            printf("[EXEC] Unknown Instruction: 0x%04X at PC 0x%08X\n", instr, current_pc);
            // For safety, stop simulation when encountering an unknown instruction
            cpu->pc = 0xFFFFFFFF; // Impossible address to stop
            break;
    }
}

void simulator_step(Simulator *sim) {
    CPU_State *cpu = &sim->cpu;
    Memory *mem = &sim->mem;

    // 1. FETCH: Проверяем, что PC находится в допустимом диапазоне Flash
    if (cpu->pc < FLASH_BASE_ADDR || cpu->pc >= (FLASH_BASE_ADDR + FLASH_SIZE)) {
        printf("[ERROR] PC out of Flash range: 0x%08X\n", cpu->pc);
        cpu->pc = 0xFFFFFFFF; // Невероятный адрес для останова
        return;
    }
    
    // Проверяем, что при чтении halfword не выйдем за границу
    if (cpu->pc + 1 >= (FLASH_BASE_ADDR + FLASH_SIZE)) {
        printf("[ERROR] PC would cause halfword read out of Flash range: 0x%08X\n", cpu->pc);
        cpu->pc = 0xFFFFFFFF; // Невероятный адрес для останова
        return;
    }
    
    // Считываем 16-битную инструкцию по текущему PC
    uint16_t instr = memory_read_halfword(mem, cpu->pc);
    
    // Увеличиваем PC (в Thumb режиме PC увеличивается на 2 после чтения)
    uint32_t current_pc = cpu->pc;
    cpu->pc += 2;

    // 2. DECODE & EXECUTE
    // Определяем тип инструкции по старшим битам
    
    // Для 16-битных инструкций определяем тип по маске
    uint16_t opcode = get_bits(instr, 11, 15);

    // В Thumb-16 только branch инструкции могут иметь condition code
    // Большинство инструкций выполняются всегда (AL condition)
    // Проверяем условие только для conditional branch (opcode 0b11010)
    uint8_t condition_met = 1;
    if (opcode == 0b11010) { // Conditional branch
        condition_met = check_condition(instr, cpu);
    }

    // Если условие не выполняется, пропускаем выполнение
    if (!condition_met) {
        return;
    }

    // Для 32-битных инструкций (в будущем)
    // uint32_t instr32 = memory_read_word(mem, cpu->pc);

    switch (opcode) {
        case 0b00000: { // ADD Rd, Rn, #imm3
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm3 = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] + imm3;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], imm3, result, 0);
            printf("[EXEC] ADD R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rn, imm3, rd, cpu->regs[rd]);
            break;
        }

        case 0b00001: { // SUB Rd, Rn, #imm3
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm3 = get_bits(instr, 6, 8);
            uint32_t result = cpu->regs[rn] - imm3;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], imm3, result, 1);
            printf("[EXEC] SUB R%u, R%u, #%u -> R%u = 0x%08X\n", rd, rn, imm3, rd, cpu->regs[rd]);
            break;
        }

        case 0b00010: { // ADD Rd, #imm8
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t result = cpu->regs[rd] + imm8;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rd], imm8, result, 0);
            printf("[EXEC] ADD R%u, #0x%02X -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b00011: { // Group: ADD/SUB/CMP/MOV register-register
            uint16_t op = get_bits(instr, 9, 10);
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);

            if (op == 0b00) { // ADD Rd, Rn, Rm
                uint32_t result = cpu->regs[rn] + cpu->regs[rm];
                cpu->regs[rd] = result;
                update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 0);
                printf("[EXEC] ADD R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            } else if (op == 0b01) { // SUB Rd, Rn, Rm
                uint32_t result = cpu->regs[rn] - cpu->regs[rm];
                cpu->regs[rd] = result;
                update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 1);
                printf("[EXEC] SUB R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            } else if (op == 0b10) { // CMP Rn, Rm
                uint32_t result = cpu->regs[rn] - cpu->regs[rm];
                update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 1);
                printf("[EXEC] CMP R%u, R%u\n", rn, rm);
            } else { // op == 0b11: MOV Rd, Rm
                cpu->regs[rd] = cpu->regs[rm];
                update_flags(cpu, cpu->regs[rd], 0);
                printf("[EXEC] MOV R%u, R%u -> R%u = 0x%08X\n", rd, rm, rd, cpu->regs[rd]);
            }
            break;
        }

        case 0b00100: { // MOV Rd, #imm8
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            cpu->regs[rd] = imm8;
            update_flags(cpu, imm8, 0);
            printf("[EXEC] MOV R%u, #0x%02X -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b00101: { // SUB Rd, #imm8
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t result = cpu->regs[rd] - imm8;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rd], imm8, result, 1);
            printf("[EXEC] SUB R%u, #0x%02X -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b00110: { // ADD Rd, Rn, #imm8
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t result = cpu->regs[rn] + imm8;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], imm8, result, 0);
            printf("[EXEC] ADD R%u, R%u, #0x%02X -> R%u = 0x%08X\n", rd, rn, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b00111: { // SUB Rd, Rn, #imm8
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t result = cpu->regs[rn] - imm8;
            cpu->regs[rd] = result;
            update_arithmetic_flags(cpu, cpu->regs[rn], imm8, result, 1);
            printf("[EXEC] SUB R%u, R%u, #0x%02X -> R%u = 0x%08X\n", rd, rn, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b01000: { // TST/NEG/CMP/CMN group
            uint16_t op = get_bits(instr, 8, 9);
            uint32_t rm = get_bits(instr, 3, 5);
            uint32_t rn = get_bits(instr, 0, 2);

            if (op == 0b00) { // AND Rd, Rm (Rd = Rn для совместимости)
                uint32_t rd = rn;
                cpu->regs[rd] = cpu->regs[rn] & cpu->regs[rm];
                update_flags(cpu, cpu->regs[rd], 0);
                printf("[EXEC] AND R%u, R%u -> R%u = 0x%08X\n", rd, rm, rd, cpu->regs[rd]);
            } else if (op == 0b01) { // EOR Rd, Rm
                uint32_t rd = rn;
                cpu->regs[rd] = cpu->regs[rn] ^ cpu->regs[rm];
                update_flags(cpu, cpu->regs[rd], 0);
                printf("[EXEC] EOR R%u, R%u -> R%u = 0x%08X\n", rd, rm, rd, cpu->regs[rd]);
            } else if (op == 0b10) { // LSL Rd, Rm
                uint32_t rd = rn;
                cpu->regs[rd] = cpu->regs[rn] << cpu->regs[rm];
                update_flags(cpu, cpu->regs[rd], 0);
                printf("[EXEC] LSL R%u, R%u -> R%u = 0x%08X\n", rd, rm, rd, cpu->regs[rd]);
            } else { // op == 0b11: NEG Rd, Rm
                uint32_t rd = rn;
                cpu->regs[rd] = 0 - cpu->regs[rm];
                update_arithmetic_flags(cpu, 0, cpu->regs[rm], cpu->regs[rd], 1);
                printf("[EXEC] NEG R%u, R%u -> R%u = 0x%08X\n", rd, rm, rd, cpu->regs[rd]);
            }
            break;
        }

        case 0b01001: { // ADC/SBC/ROR/MUL group
            uint16_t op = get_bits(instr, 8, 9);
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);

            if (op == 0b00) { // ADC Rd, Rn, Rm
                uint32_t carry = (cpu->xpsr & 0x100) ? 1 : 0;
                uint32_t result = cpu->regs[rn] + cpu->regs[rm] + carry;
                cpu->regs[rd] = result;
                update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 0);
                printf("[EXEC] ADC R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            } else if (op == 0b01) { // SBC Rd, Rn, Rm
                uint32_t carry = (cpu->xpsr & 0x100) ? 0 : 1;
                uint32_t result = cpu->regs[rn] - cpu->regs[rm] - carry;
                cpu->regs[rd] = result;
                update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], result, 1);
                printf("[EXEC] SBC R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            } else if (op == 0b10) { // ROR Rd, Rn, Rm
                uint32_t shift = cpu->regs[rm] & 0x1F;
                cpu->regs[rd] = (cpu->regs[rn] >> shift) | (cpu->regs[rn] << (32 - shift));
                update_flags(cpu, cpu->regs[rd], 0);
                printf("[EXEC] ROR R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            } else { // op == 0b11: MUL Rd, Rn, Rm
                cpu->regs[rd] = cpu->regs[rn] * cpu->regs[rm];
                update_flags(cpu, cpu->regs[rd], 0);
                printf("[EXEC] MUL R%u, R%u, R%u -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            }
            break;
        }

        case 0b01010: { // STR Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [R%u, R%u]\n", rd, rn, rm);
            break;
        }
            printf("[EXEC] STR R%u, [R%u, R%u]\n", rd, rn, rm);
            break;
        }

        case 0b01011: { // STRH Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            memory_write_halfword(mem, addr, cpu->regs[rd] & 0xFFFF);
            printf("[EXEC] STRH R%u, [R%u, R%u]\n", rd, rn, rm);
            break;
        }

        case 0b01100: { // STRB Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            memory_write_byte(mem, addr, cpu->regs[rd] & 0xFF);
            printf("[EXEC] STRB R%u, [R%u, R%u]\n", rd, rn, rm);
            break;
        }

        case 0b01101: { // LDR Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            cpu->regs[rd] = memory_read_word(mem, addr);
            printf("[EXEC] LDR R%u, [R%u, R%u] -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            break;
        }

        case 0b01110: { // LDRB Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            cpu->regs[rd] = memory_read_byte(mem, addr);
            printf("[EXEC] LDRB R%u, [R%u, R%u] -> R%u = 0x%02X\n", rd, rn, rm, rd, cpu->regs[rd]);
            break;
        }

        case 0b01111: { // LDRH Rd, [Rn, Rm]
            uint32_t rm = get_bits(instr, 6, 8);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
            cpu->regs[rd] = memory_read_halfword(mem, addr);
            printf("[EXEC] LDRH R%u, [R%u, R%u] -> R%u = 0x%08X\n", rd, rn, rm, rd, cpu->regs[rd]);
            break;
        }

        case 0b10000: { // STRB Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + imm5;
            memory_write_byte(mem, addr, cpu->regs[rd] & 0xFF);
            printf("[EXEC] STRB R%u, [R%u, #%u]\n", rd, rn, imm5);
            break;
        }

        case 0b10001: { // LDRB Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + imm5;
            cpu->regs[rd] = memory_read_byte(mem, addr);
            printf("[EXEC] LDRB R%u, [R%u, #%u] -> R%u = 0x%08X\n", rd, rn, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b10010: { // STRH Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + (imm5 << 1);
            memory_write_halfword(mem, addr, cpu->regs[rd] & 0xFFFF);
            printf("[EXEC] STRH R%u, [R%u, #%u]\n", rd, rn, imm5);
            break;
        }

        case 0b10011: { // LDRH Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + (imm5 << 1);
            cpu->regs[rd] = memory_read_halfword(mem, addr);
            printf("[EXEC] LDRH R%u, [R%u, #%u] -> R%u = 0x%08X\n", rd, rn, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b10100: { // STR Rd, [SP, #imm8]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t addr = cpu->regs[13] + (imm8 << 2);
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [SP, #%u]\n", rd, imm8);
            break;
        }

        case 0b10101: { // LDR Rd, [SP, #imm8]
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t addr = cpu->regs[13] + (imm8 << 2);
            cpu->regs[rd] = memory_read_word(mem, addr);
            printf("[EXEC] LDR R%u, [SP, #%u] -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b10110: { // ADD Rd, PC, #imm8
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            uint32_t addr = (cpu->pc & ~0x3U) + (imm8 << 2);
            cpu->regs[rd] = memory_read_word(mem, addr);
            printf("[EXEC] LDR R%u, [PC, #%u] -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b10111: { // ADD Rd, SP, #imm8
            uint32_t rd = get_bits(instr, 8, 10);
            uint32_t imm8 = get_bits(instr, 0, 7);
            cpu->regs[rd] = cpu->regs[13] + (imm8 << 2);
            printf("[EXEC] ADD R%u, SP, #%u -> R%u = 0x%08X\n", rd, imm8, rd, cpu->regs[rd]);
            break;
        }

        case 0b11000: { // STR Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + (imm5 << 2);
            if (!memory_write_word_safe(mem, addr, cpu->regs[rd])) {
                printf("[ERROR] Invalid memory write at address 0x%08X\n", addr);
                cpu->pc = 0xFFFFFFFF; // Ошибка выполнения
                break;
            }
            printf("[EXEC] STR R%u, [R%u, #%u]\n", rd, rn, imm5);
            break;
        }
            printf("[EXEC] STR R%u, [R%u, #%u]\n", rd, rn, imm5);
            break;
        }

        case 0b11001: { // LDR Rd, [Rn, #imm5]
            uint32_t rd = get_bits(instr, 0, 2);
            uint32_t rn = get_bits(instr, 3, 5);
            uint32_t imm5 = get_bits(instr, 6, 10);
            uint32_t addr = cpu->regs[rn] + (imm5 << 2);
            // Проверяем, что адрес в допустимом диапазоне для чтения
            if ((addr >= FLASH_BASE_ADDR && addr + 3 < (FLASH_BASE_ADDR + FLASH_SIZE)) ||
                (addr >= SRAM_BASE_ADDR && addr + 3 < (SRAM_BASE_ADDR + SRAM_SIZE))) {
                cpu->regs[rd] = memory_read_word(mem, addr);
            } else {
                printf("[ERROR] Invalid memory read at address 0x%08X\n", addr);
                cpu->regs[rd] = 0xFFFFFFFF; // Значение ошибки
            }
            printf("[EXEC] LDR R%u, [R%u, #%u] -> R%u = 0x%08X\n", rd, rn, imm5, rd, cpu->regs[rd]);
            break;
        }

        case 0b11010: { // Conditional branch (B cond)
            int32_t offset = sign_extend(get_bits(instr, 0, 7), 8);
            uint32_t target = cpu->pc + (offset << 1);
            cpu->pc = target;
            printf("[EXEC] B.cond (Offset: %d)\n", offset);
            break;
        }

        case 0b11011: { // SVC / SWI
            printf("[EXEC] SVC instruction at PC 0x%08X\n", current_pc);
            break;
        }

        case 0b11100: { // Unconditional Branch (B)
            int32_t offset = sign_extend(get_bits(instr, 0, 10), 11);
            uint32_t target = cpu->pc + (offset << 1);
            printf("[EXEC] B 0x%08X (Offset: %d)\n", target, offset);
            cpu->pc = target;
            break;
        }

        case 0b11101: { // BL / BLX first half (32-bit)
            printf("[EXEC] BL/BLX first half (32-bit instruction not fully supported)\n");
            break;
        }

        case 0b11110: { // BL / BLX second half (32-bit)
            printf("[EXEC] BL/BLX second half (32-bit instruction not fully supported)\n");
            break;
        }

        case 0b11111: { // BX Rd
            uint32_t rd = get_bits(instr, 3, 5);
            uint32_t target = cpu->regs[rd];
            cpu->pc = target & ~0x1U;
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