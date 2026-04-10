/**
 * execute.c - Thumb-16 Instruction Decoder and Executor
 *
 * Implements the Cortex-M3 Thumb-16 instruction set according to
 * ARM Architecture Reference Manual (DDI0403D).
 *
 * Supported instruction groups:
 *   - Data processing: ADD, SUB, MOV, CMP, AND, EOR, LSL, NEG, ADC, SBC, ROR, MUL
 *   - Memory access:  LDR, STR, LDRB, LDRH, STRB, STRH (register and immediate offset)
 *   - Branch:         B (unconditional), B.cond (conditional), BX
 */

#include "execute.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Bit manipulation helpers                                            */
/* ------------------------------------------------------------------ */

/** Extract bits [start..end] (inclusive, 0-based) from value. */
static inline uint32_t get_bits(uint32_t value, int start, int end) {
    uint32_t mask = (1U << (end - start + 1)) - 1;
    return (value >> start) & mask;
}

/** Sign-extend a value of `bits` width to 32-bit signed integer. */
static inline int32_t sign_extend(uint32_t value, int bits) {
    uint32_t sign_bit = 1U << (bits - 1);
    if (value & sign_bit) {
        return (int32_t)(value | (~0U << bits));
    }
    return (int32_t)value;
}

/* ------------------------------------------------------------------ */
/*  CPSR flag helpers                                                   */
/* ------------------------------------------------------------------ */

/** Update N (Negative) and Z (Zero) flags based on result. */
static void update_flags(CPU_State *cpu, uint32_t result) {
    cpu->xpsr = (cpu->xpsr & ~0x80000000U) | (result & 0x80000000U ? 0x80000000U : 0);
    cpu->xpsr = (cpu->xpsr & ~0x40000000U) | (result == 0 ? 0x40000000U : 0);
}

/** Update N, Z, C, V flags after an arithmetic operation. */
static void update_arithmetic_flags(CPU_State *cpu, uint32_t a, uint32_t b,
                                     uint32_t result, uint8_t is_subtract) {
    /* N flag */
    cpu->xpsr = (cpu->xpsr & ~0x80000000U) | (result & 0x80000000U ? 0x80000000U : 0);
    /* Z flag */
    cpu->xpsr = (cpu->xpsr & ~0x40000000U) | (result == 0 ? 0x40000000U : 0);
    /* C flag (Carry) */
    if (is_subtract) {
        cpu->xpsr = (cpu->xpsr & ~0x100U) | (a >= b ? 0x100U : 0);
    } else {
        cpu->xpsr = (cpu->xpsr & ~0x100U) | (result < a ? 0x100U : 0);
    }
    /* V flag (Overflow) for signed arithmetic */
    uint8_t a_sign = (a & 0x80000000U) ? 1 : 0;
    uint8_t b_sign = (b & 0x80000000U) ? 1 : 0;
    uint8_t result_sign = (result & 0x80000000U) ? 1 : 0;
    cpu->xpsr = (cpu->xpsr & ~0x200U) |
                (((a_sign ^ b_sign) & (a_sign ^ result_sign)) ? 0x200U : 0);
}

/* ------------------------------------------------------------------ */
/*  Conditional branch evaluation                                       */
/* ------------------------------------------------------------------ */

/**
 * Evaluate the condition code for a conditional branch instruction.
 * In Thumb-16, the condition code occupies bits 8-11.
 */
static uint8_t check_condition(uint16_t instr, CPU_State *cpu) {
    uint8_t cond = get_bits(instr, 8, 11);

    uint8_t n = (cpu->xpsr & 0x80000000U) ? 1 : 0;
    uint8_t z = (cpu->xpsr & 0x40000000U) ? 1 : 0;
    uint8_t c = (cpu->xpsr & 0x100U) ? 1 : 0;
    uint8_t v = (cpu->xpsr & 0x200U) ? 1 : 0;

    switch (cond) {
        case 0b0000: return z;             /* EQ */
        case 0b0001: return !z;            /* NE */
        case 0b0010: return c;             /* CS/HS */
        case 0b0011: return !c;            /* CC/LO */
        case 0b0100: return n;             /* MI */
        case 0b0101: return !n;            /* PL */
        case 0b0110: return v;             /* VS */
        case 0b0111: return !v;            /* VC */
        case 0b1000: return c && !z;       /* HI */
        case 0b1001: return !c || z;       /* LS */
        case 0b1010: return n == v;        /* GE */
        case 0b1011: return n != v;        /* LT */
        case 0b1100: return !z && n == v;  /* GT */
        case 0b1101: return z || n != v;   /* LE */
        case 0b1110: return 1;             /* AL */
        default:     return 0;             /* NV (reserved in Thumb) */
    }
}

/* ------------------------------------------------------------------ */
/*  Single-step execution                                               */
/* ------------------------------------------------------------------ */

void simulator_step(Simulator *sim) {
    CPU_State *cpu = &sim->cpu;
    Memory   *mem = &sim->mem;

    /* 1. FETCH: read 16-bit instruction from current PC */
    uint16_t instr = memory_read_halfword(mem, cpu->pc);
    uint32_t fetch_pc = cpu->pc;   /* save for diagnostics */
    cpu->pc += 2;                  /* Thumb PC advances after fetch */

    /* 2. DECODE: extract opcode (bits 15-11) */
    uint16_t opcode = get_bits(instr, 11, 15);

    /* 3. EXECUTE — conditional branch check is inside the branch case */

    /* 4. EXECUTE */
    switch (opcode) {

    /* ================================================================ */
    /*  Data processing – immediate                                       */
    /* ================================================================ */

    case 0b00000: {  /* ADDS Rd, Rn, #imm3 */
        uint32_t rd  = get_bits(instr, 0, 2);
        uint32_t rn  = get_bits(instr, 3, 5);
        uint32_t imm = get_bits(instr, 6, 8);
        uint32_t res = cpu->regs[rn] + imm;
        cpu->regs[rd] = res;
        update_arithmetic_flags(cpu, cpu->regs[rn], imm, res, 0);
        break;
    }

    case 0b00001: {  /* SUBS Rd, Rn, #imm3 */
        uint32_t rd  = get_bits(instr, 0, 2);
        uint32_t rn  = get_bits(instr, 3, 5);
        uint32_t imm = get_bits(instr, 6, 8);
        uint32_t res = cpu->regs[rn] - imm;
        cpu->regs[rd] = res;
        update_arithmetic_flags(cpu, cpu->regs[rn], imm, res, 1);
        break;
    }

    case 0b00010: {  /* ADDS Rd, #imm8 */
        uint32_t rd  = get_bits(instr, 8, 10);
        uint32_t imm = get_bits(instr, 0, 7);
        uint32_t res = cpu->regs[rd] + imm;
        cpu->regs[rd] = res;
        update_arithmetic_flags(cpu, cpu->regs[rd], imm, res, 0);
        break;
    }

    case 0b00100: {  /* MOVS Rd, #imm8 */
        uint32_t rd  = get_bits(instr, 8, 10);
        uint32_t imm = get_bits(instr, 0, 7);
        cpu->regs[rd] = imm;
        update_flags(cpu, imm);
        break;
    }

    case 0b00101: {  /* SUBS Rd, #imm8 */
        uint32_t rd  = get_bits(instr, 8, 10);
        uint32_t imm = get_bits(instr, 0, 7);
        uint32_t res = cpu->regs[rd] - imm;
        cpu->regs[rd] = res;
        update_arithmetic_flags(cpu, cpu->regs[rd], imm, res, 1);
        break;
    }

    /* ================================================================ */
    /*  Data processing – register                                        */
    /* ================================================================ */

    case 0b00011: {  /* ADDS/SUBS/CMP/MOVS register group */
        uint16_t op = get_bits(instr, 9, 10);
        uint32_t rm = get_bits(instr, 6, 8);
        uint32_t rn = get_bits(instr, 3, 5);
        uint32_t rd = get_bits(instr, 0, 2);

        if (op == 0b00) {          /* ADDS Rd, Rn, Rm */
            uint32_t res = cpu->regs[rn] + cpu->regs[rm];
            cpu->regs[rd] = res;
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], res, 0);
        } else if (op == 0b01) {   /* SUBS Rd, Rn, Rm */
            uint32_t res = cpu->regs[rn] - cpu->regs[rm];
            cpu->regs[rd] = res;
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], res, 1);
        } else if (op == 0b10) {   /* CMP Rn, Rm */
            uint32_t res = cpu->regs[rn] - cpu->regs[rm];
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], res, 1);
        } else {                   /* MOVS Rd, Rm */
            cpu->regs[rd] = cpu->regs[rm];
            update_flags(cpu, cpu->regs[rd]);
        }
        break;
    }

    case 0b00110: {  /* ADDS Rd, Rn, #imm8 */
        uint32_t rd  = get_bits(instr, 8, 10);
        uint32_t rn  = get_bits(instr, 3, 5);
        uint32_t imm = get_bits(instr, 0, 7);
        uint32_t res = cpu->regs[rn] + imm;
        cpu->regs[rd] = res;
        update_arithmetic_flags(cpu, cpu->regs[rn], imm, res, 0);
        break;
    }

    case 0b00111: {  /* SUBS Rd, Rn, #imm8 */
        uint32_t rd  = get_bits(instr, 8, 10);
        uint32_t rn  = get_bits(instr, 3, 5);
        uint32_t imm = get_bits(instr, 0, 7);
        uint32_t res = cpu->regs[rn] - imm;
        cpu->regs[rd] = res;
        update_arithmetic_flags(cpu, cpu->regs[rn], imm, res, 1);
        break;
    }

    /* ================================================================ */
    /*  Data processing – extended                                        */
    /* ================================================================ */

    case 0b01000: {  /* AND/EOR/LSL/NEG group */
        uint16_t op = get_bits(instr, 8, 9);
        uint32_t rm = get_bits(instr, 3, 5);
        uint32_t rn = get_bits(instr, 0, 2);
        uint32_t rd = rn;   /* destination is always Rd = Rn for this group */

        if (op == 0b00) {          /* ANDS Rd, Rm */
            cpu->regs[rd] &= cpu->regs[rm];
            update_flags(cpu, cpu->regs[rd]);
        } else if (op == 0b01) {   /* EORS Rd, Rm */
            cpu->regs[rd] ^= cpu->regs[rm];
            update_flags(cpu, cpu->regs[rd]);
        } else if (op == 0b10) {   /* LSLS Rd, Rm */
            uint32_t shift = cpu->regs[rm] & 0x1F;
            cpu->regs[rd] <<= shift;
            update_flags(cpu, cpu->regs[rd]);
        } else {                   /* NEGS Rd, Rm  (RSB Rd, Rm, #0) */
            uint32_t res = 0 - cpu->regs[rm];
            cpu->regs[rd] = res;
            update_arithmetic_flags(cpu, 0, cpu->regs[rm], res, 1);
        }
        break;
    }

    case 0b01001: {  /* ADC/SBC/ROR/MUL group */
        uint16_t op = get_bits(instr, 8, 9);
        uint32_t rm = get_bits(instr, 6, 8);
        uint32_t rn = get_bits(instr, 3, 5);
        uint32_t rd = get_bits(instr, 0, 2);

        if (op == 0b00) {          /* ADCS Rd, Rn, Rm */
            uint32_t carry = (cpu->xpsr & 0x100U) ? 1 : 0;
            uint32_t res = cpu->regs[rn] + cpu->regs[rm] + carry;
            cpu->regs[rd] = res;
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], res, 0);
        } else if (op == 0b01) {   /* SBCS Rd, Rn, Rm */
            uint32_t carry = (cpu->xpsr & 0x100U) ? 0 : 1;
            uint32_t res = cpu->regs[rn] - cpu->regs[rm] - carry;
            cpu->regs[rd] = res;
            update_arithmetic_flags(cpu, cpu->regs[rn], cpu->regs[rm], res, 1);
        } else if (op == 0b10) {   /* RORS Rd, Rn, Rm */
            uint32_t shift = cpu->regs[rm] & 0x1F;
            if (shift == 0) {
                cpu->regs[rd] = cpu->regs[rn];
            } else {
                cpu->regs[rd] = (cpu->regs[rn] >> shift) |
                                (cpu->regs[rn] << (32 - shift));
            }
            update_flags(cpu, cpu->regs[rd]);
        } else {                   /* MULS Rd, Rn, Rm */
            cpu->regs[rd] = cpu->regs[rn] * cpu->regs[rm];
            update_flags(cpu, cpu->regs[rd]);
        }
        break;
    }

    /* ================================================================ */
    /*  Load / Store – register offset                                    */
    /* ================================================================ */

    case 0b01010: {  /* STR Rd, [Rn, Rm] */
        uint32_t rm   = get_bits(instr, 6, 8);
        uint32_t rn   = get_bits(instr, 3, 5);
        uint32_t rd   = get_bits(instr, 0, 2);
        uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
        memory_write_word(mem, addr, cpu->regs[rd]);
        break;
    }

    case 0b01011: {  /* STRH Rd, [Rn, Rm] */
        uint32_t rm   = get_bits(instr, 6, 8);
        uint32_t rn   = get_bits(instr, 3, 5);
        uint32_t rd   = get_bits(instr, 0, 2);
        uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
        memory_write_halfword(mem, addr, cpu->regs[rd] & 0xFFFFU);
        break;
    }

    case 0b01100: {  /* STRB Rd, [Rn, Rm] */
        uint32_t rm   = get_bits(instr, 6, 8);
        uint32_t rn   = get_bits(instr, 3, 5);
        uint32_t rd   = get_bits(instr, 0, 2);
        uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
        memory_write_byte(mem, addr, cpu->regs[rd] & 0xFFU);
        break;
    }

    case 0b01101: {  /* LDR Rd, [Rn, Rm] */
        uint32_t rm   = get_bits(instr, 6, 8);
        uint32_t rn   = get_bits(instr, 3, 5);
        uint32_t rd   = get_bits(instr, 0, 2);
        uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
        cpu->regs[rd] = memory_read_word(mem, addr);
        break;
    }

    case 0b01110: {  /* LDRB Rd, [Rn, Rm] */
        uint32_t rm   = get_bits(instr, 6, 8);
        uint32_t rn   = get_bits(instr, 3, 5);
        uint32_t rd   = get_bits(instr, 0, 2);
        uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
        cpu->regs[rd] = memory_read_byte(mem, addr);
        break;
    }

    case 0b01111: {  /* LDRH Rd, [Rn, Rm] */
        uint32_t rm   = get_bits(instr, 6, 8);
        uint32_t rn   = get_bits(instr, 3, 5);
        uint32_t rd   = get_bits(instr, 0, 2);
        uint32_t addr = cpu->regs[rn] + cpu->regs[rm];
        cpu->regs[rd] = memory_read_halfword(mem, addr);
        break;
    }

    /* ================================================================ */
    /*  Load / Store – immediate offset                                   */
    /* ================================================================ */

    case 0b10000: {  /* STRB Rd, [Rn, #imm5] */
        uint32_t rd   = get_bits(instr, 0, 2);
        uint32_t rn   = get_bits(instr, 3, 5);
        uint32_t imm  = get_bits(instr, 6, 10);
        memory_write_byte(mem, cpu->regs[rn] + imm, cpu->regs[rd] & 0xFFU);
        break;
    }

    case 0b10001: {  /* LDRB Rd, [Rn, #imm5] */
        uint32_t rd  = get_bits(instr, 0, 2);
        uint32_t rn  = get_bits(instr, 3, 5);
        uint32_t imm = get_bits(instr, 6, 10);
        cpu->regs[rd] = memory_read_byte(mem, cpu->regs[rn] + imm);
        break;
    }

    case 0b10010: {  /* STRH Rd, [Rn, #imm5]  (halfword-aligned) */
        uint32_t rd   = get_bits(instr, 0, 2);
        uint32_t rn   = get_bits(instr, 3, 5);
        uint32_t imm  = get_bits(instr, 6, 10);
        memory_write_halfword(mem, cpu->regs[rn] + (imm << 1),
                              cpu->regs[rd] & 0xFFFFU);
        break;
    }

    case 0b10011: {  /* LDRH Rd, [Rn, #imm5] */
        uint32_t rd  = get_bits(instr, 0, 2);
        uint32_t rn  = get_bits(instr, 3, 5);
        uint32_t imm = get_bits(instr, 6, 10);
        cpu->regs[rd] = memory_read_halfword(mem, cpu->regs[rn] + (imm << 1));
        break;
    }

    case 0b10100: {  /* STR Rd, [SP, #imm8] */
        uint32_t rd   = get_bits(instr, 8, 10);
        uint32_t imm  = get_bits(instr, 0, 7);
        memory_write_word(mem, cpu->regs[13] + (imm << 2), cpu->regs[rd]);
        break;
    }

    case 0b10101: {  /* LDR Rd, [SP, #imm8] */
        uint32_t rd  = get_bits(instr, 8, 10);
        uint32_t imm = get_bits(instr, 0, 7);
        cpu->regs[rd] = memory_read_word(mem, cpu->regs[13] + (imm << 2));
        break;
    }

    /* ================================================================ */
    /*  PC / SP relative load                                             */
    /* ================================================================ */

    case 0b10110: {  /* LDR Rd, [PC, #imm8] */
        uint32_t rd  = get_bits(instr, 8, 10);
        uint32_t imm = get_bits(instr, 0, 7);
        uint32_t addr = (cpu->pc & ~0x3U) + (imm << 2);
        cpu->regs[rd] = memory_read_word(mem, addr);
        break;
    }

    case 0b10111: {  /* ADD Rd, SP, #imm8 */
        uint32_t rd  = get_bits(instr, 8, 10);
        uint32_t imm = get_bits(instr, 0, 7);
        cpu->regs[rd] = cpu->regs[13] + (imm << 2);
        break;
    }

    /* ================================================================ */
    /*  Load / Store – word immediate (full word)                         */
    /* ================================================================ */

    case 0b11000: {  /* STR Rd, [Rn, #imm5]  (word-aligned) */
        uint32_t rd  = get_bits(instr, 0, 2);
        uint32_t rn  = get_bits(instr, 3, 5);
        uint32_t imm = get_bits(instr, 6, 10);
        memory_write_word(mem, cpu->regs[rn] + (imm << 2), cpu->regs[rd]);
        break;
    }

    case 0b11001: {  /* LDR Rd, [Rn, #imm5] */
        uint32_t rd  = get_bits(instr, 0, 2);
        uint32_t rn  = get_bits(instr, 3, 5);
        uint32_t imm = get_bits(instr, 6, 10);
        cpu->regs[rd] = memory_read_word(mem, cpu->regs[rn] + (imm << 2));
        break;
    }

    /* ================================================================ */
    /*  Branch                                                            */
    /* ================================================================ */

    case 0b11010:  /* B.cond (condition 0xxx: EQ..VC) */
    case 0b11011: { /* B.cond (condition 1xxx: HI..LE) or SVC (1111) */
        uint8_t cond = get_bits(instr, 8, 11);

        if (cond == 0b1111) {
            /* SVC / SWI – not implemented */
            fprintf(stderr, "[EXEC] SVC at PC 0x%08X – halting\n", fetch_pc);
            cpu->pc = 0xFFFFFFFFU;
            break;
        }

        /* Conditional branch */
        if (!check_condition(instr, cpu)) {
            break;  /* condition not met – fall through */
        }
        int32_t offset = sign_extend(get_bits(instr, 0, 7), 8);
        cpu->pc = cpu->pc + (offset << 1);
        break;
    }

    case 0b11100: {  /* B label  (unconditional branch) */
        int32_t offset = sign_extend(get_bits(instr, 0, 10), 11);
        cpu->pc = cpu->pc + (offset << 1);
        break;
    }

    case 0b11101:  /* BL / BLX first halfword (32-bit) – stub */
    case 0b11110:  /* BL / BLX second halfword (32-bit) – stub */
        fprintf(stderr, "[EXEC] BL/BLX at PC 0x%08X – not supported\n", fetch_pc);
        break;

    case 0b11111: {  /* BX Rm */
        uint32_t rm = get_bits(instr, 3, 5);
        cpu->pc = cpu->regs[rm] & ~0x1U;   /* clear Thumb bit */
        break;
    }

    /* ================================================================ */
    /*  Unknown instruction                                               */
    /* ================================================================ */

    default:
        fprintf(stderr, "[EXEC] Unknown instruction 0x%04X at PC 0x%08X – halting\n",
                instr, fetch_pc);
        cpu->pc = 0xFFFFFFFFU;
        break;
    }
}

/**
 * Run the simulator until PC leaves Flash range or an invalid instruction
 * is encountered.  Uses a configurable step limit to prevent infinite loops.
 */
void simulator_run(Simulator *sim, int max_steps) {
    int steps = 0;
    while (sim->cpu.pc >= FLASH_BASE_ADDR &&
           sim->cpu.pc < FLASH_BASE_ADDR + FLASH_SIZE &&
           sim->cpu.pc != 0xFFFFFFFFU &&
           steps < max_steps) {
        simulator_step(sim);
        steps++;
    }
    if (steps >= max_steps) {
        printf("[SIM] Step limit (%d) reached.\n", max_steps);
    }
}
