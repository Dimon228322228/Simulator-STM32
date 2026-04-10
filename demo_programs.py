#!/usr/bin/env python3
"""
demo_programs.py — Generate Thumb-16 binary demo programs for STM32 Simulator

Each function returns a list of 16-bit instructions (little-endian when packed).
Run with: python3 demo_programs.py all
"""

import struct
import sys
import os


def pack(program, name):
    """Pack a list of 16-bit integers into a .bin file."""
    data = struct.pack('<' + 'H' * len(program), *program)
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), name + '.bin')
    with open(path, 'wb') as f:
        f.write(data)
    print(f"  Created {name}.bin  ({len(program)} instructions, {len(data)} bytes)")
    return path


def cond_branch(cond, offset):
    """Build a conditional branch instruction B<cond> with 8-bit signed offset."""
    imm = offset & 0xFF
    return (0b11010 << 11) | (cond << 8) | imm


# Condition codes for Thumb-16 conditional branches
EQ = 0b0000   # Equal (Z=1)
NE = 0b0001   # Not equal (Z=0)
CS = 0b0010   # Carry set / unsigned higher or same
CC = 0b0011   # Carry clear / unsigned lower
MI = 0b0100   # Minus / negative (N=1)
PL = 0b0101   # Plus / positive or zero (N=0)
VS = 0b0110   # Overflow set
VC = 0b0111   # Overflow clear
HI = 0b1000   # Unsigned higher (C=1 and Z=0)
LS = 0b1001   # Unsigned lower or same (C=0 or Z=1)
GE = 0b1010   # Signed greater or equal (N=V)
LT = 0b1011   # Signed less than (N!=V)
GT = 0b1100   # Signed greater than (Z=0 and N=V)
LE = 0b1101   # Signed less or equal (Z=1 or N!=V)


def thumb_add_rd_rn_rm(rd, rn, rm):
    """ADDS Rd, Rn, Rm  (opcode 00011, op=00)"""
    return (0b00011 << 11) | (0b00 << 9) | (rm << 6) | (rn << 3) | rd


def thumb_sub_rd_rn_rm(rd, rn, rm):
    """SUBS Rd, Rn, Rm  (opcode 00011, op=01)"""
    return (0b00011 << 11) | (0b01 << 9) | (rm << 6) | (rn << 3) | rd


def thumb_cmp_rn_rm(rn, rm):
    """CMP Rn, Rm  (opcode 00011, op=10, Rd=000)"""
    return (0b00011 << 11) | (0b10 << 9) | (rm << 6) | (rn << 3) | 0


def thumb_mov_rd_rm(rd, rm):
    """MOVS Rd, Rm  (opcode 00011, op=11)"""
    return (0b00011 << 11) | (0b11 << 9) | (rm << 6) | (rd << 3) | rd


def thumb_eor_rd_rm(rd, rm):
    """EORS Rd, Rm  (opcode 01000, op=01 at bits 8-9, Rd=Rn)"""
    return (0b01000 << 11) | (0b01 << 8) | (rm << 3) | rd


# ---------------------------------------------------------------------------
# Demo 1: Arithmetic — add, subtract, compare
# ---------------------------------------------------------------------------
def demo_arithmetic():
    """
    Demonstrates basic arithmetic and comparison:
        R0 = 10
        R1 = 3
        R2 = R0 + R1   = 13
        R3 = R0 - R1   = 7
        CMP R2, R3     (sets flags: R2 > R3)
        B   .          (loop forever)
    """
    return [
        0x200A,                                    # MOVS  R0, #10
        0x2103,                                    # MOVS  R1, #3
        thumb_add_rd_rn_rm(2, 0, 1),               # ADDS  R2, R0, R1  → 13
        thumb_sub_rd_rn_rm(3, 0, 1),               # SUBS  R3, R0, R1  → 7
        thumb_cmp_rn_rm(2, 3),                     # CMP   R2, R3
        0xE7FF,                                    # B     .
    ]


# ---------------------------------------------------------------------------
# Demo 2: Loop counter — count from 0 to 5
# ---------------------------------------------------------------------------
def demo_loop_counter():
    """
    Counts R0 from 0 to 5 using a loop:
        R0 = 0          (counter)
        R1 = 5          (limit)
    loop:
        ADDS R0, #1
        CMP  R0, R1
        BLE  loop       (branch if R0 <= R1)
        B    .          (done)
    """
    # BLE: from BLE (idx 4) back to ADDS (idx 2) = -2 instructions = -2 halfwords
    # PC after BLE fetch = addr+2 = 10. target = 4. offset = (4-10)/2 = -3
    ble = cond_branch(LE, -3)

    return [
        0x2000,       # MOVS  R0, #0         counter
        0x2105,       # MOVS  R1, #5         limit
        # loop:
        0x3001,       # ADDS  R0, #1
        thumb_cmp_rn_rm(0, 1),   # CMP   R0, R1
        ble,          # BLE   loop
        0xE7FF,       # B     .
    ]


# ---------------------------------------------------------------------------
# Demo 3: Fibonacci — compute first 8 Fibonacci numbers
# ---------------------------------------------------------------------------
def demo_fibonacci():
    """
    Computes Fibonacci sequence in registers:
        R0 = 0          (F(n-2))
        R1 = 1          (F(n-1))
        R2 = 8          (loop counter)
    loop:
        ADDS R3, R0, R1   (F(n))
        MOVS R0, R1       (shift)
        MOVS R1, R3
        SUBS R2, #1
        BNE  loop
        B    .
    """
    # BNE: from BNE (idx 7, byte 14) back to ADDS (idx 3, byte 6)
    # PC after BNE fetch = 16. target = 6. offset = (6-16)/2 = -5
    bne = cond_branch(NE, -5)

    return [
        0x2000,                                    # MOVS  R0, #0         F(n-2)
        0x2101,                                    # MOVS  R1, #1         F(n-1)
        0x2208,                                    # MOVS  R2, #8         counter
        # loop:
        thumb_add_rd_rn_rm(3, 0, 1),               # ADDS  R3, R0, R1
        thumb_mov_rd_rm(0, 1),                     # MOVS  R0, R1
        thumb_mov_rd_rm(1, 3),                     # MOVS  R1, R3
        0x2A01,                                    # SUBS  R2, #1
        bne,                                       # BNE   loop
        0xE7FF,                                    # B     .
    ]


# ---------------------------------------------------------------------------
# Demo 4: Memory operations — write and read SRAM
# ---------------------------------------------------------------------------
def demo_memory():
    """
    Demonstrates PC-relative load (literal pool):
        LDR  R0, [PC, #8]    load 0xDEAD from literal pool
        LDR  R1, [PC, #12]   load 0xBEEF from literal pool
        ADDS R2, R0, R1      add them
        B    .
        .word 0xDEAD         (at byte 8)
        .word 0x0000         (padding)
        .word 0xBEEF         (at byte 12)
    """
    # LDR R0, [PC, #8]: Rd=0, imm8=2 → addr = (PC&~3) + 8
    # At byte 0: PC=2, PC&~3=0, addr = 0+8 = 8 ✓
    ldr_r0 = (0b10110 << 11) | (2 << 0) | (0 << 8)  # 0x4802

    # LDR R1, [PC, #8]: Rd=1, imm8=2
    # At byte 2: PC=4, PC&~3=4, addr = 4+8 = 12 ✓
    ldr_r1 = (0b10110 << 11) | (2 << 0) | (1 << 8)  # 0x4902

    return [
        ldr_r0,       # LDR   R0, [PC, #8]   → loads 0xDEAD
        ldr_r1,       # LDR   R1, [PC, #8]   → loads 0xBEEF
        thumb_add_rd_rn_rm(2, 0, 1),          # ADDS  R2, R0, R1
        0xE7FF,       # B     .
        0xDEAD,       # literal at byte 8: 0xDEAD
        0x0000,       # padding at byte 10
        0xBEEF,       # literal at byte 12: 0xBEEF
    ]


# ---------------------------------------------------------------------------
# Demo 5: LED blink simulation — toggle GPIO bits
# ---------------------------------------------------------------------------
def demo_led_blink():
    """
    Simulates LED blinking by toggling a register:
        R0 = 0x01       (LED mask)
        R1 = 5          (blink counter)
        R2 = 0x00       (GPIO state)
    loop:
        EORS R2, R0     (toggle LED bits)
        SUBS R1, #1
        BNE  loop
        B    .
    """
    # BNE: from BNE (idx 5, byte 10) back to EORS (idx 3, byte 6)
    # PC after BNE fetch = 12. target = 6. offset = (6-12)/2 = -3
    bne = cond_branch(NE, -3)

    return [
        0x2001,       # MOVS  R0, #1         LED mask (bit 0)
        0x2105,       # MOVS  R1, #5         blink count
        0x2200,       # MOVS  R2, #0         GPIO state
        # loop:
        thumb_eor_rd_rm(2, 0),               # EORS  R2, R0
        0x2901,       # SUBS  R1, #1
        bne,          # BNE   loop
        0xE7FF,       # B     .
    ]


# ---------------------------------------------------------------------------
# Demo 6: Maximum — find larger of two values
# ---------------------------------------------------------------------------
def demo_max_value():
    """
    Finds the maximum of two values:
        R0 = 42
        R1 = 17
        CMP  R0, R1
        BGT  r0_is_max
        MOVS R2, R1     (R1 is max)
        B    done
    r0_is_max:
        MOVS R2, R0     (R0 is max)
    done:
        B    .
    """
    # BGT: from BGT (idx 3, byte 6) to r0_is_max (idx 6, byte 12)
    # PC after BGT fetch = 8. target = 12. offset = (12-8)/2 = 2
    bgt = cond_branch(GT, 2)

    # B done: from B (idx 5, byte 10) to done (idx 7, byte 14)
    # PC after B fetch = 12. target = 14. offset = (14-12)/2 = 1
    b_done = cond_branch(0b1110, 1)  # B.AL (unconditional via conditional encoding)

    return [
        0x202A,                                    # MOVS  R0, #42
        0x2111,                                    # MOVS  R1, #17
        thumb_cmp_rn_rm(0, 1),                     # CMP   R0, R1
        bgt,                                       # BGT   r0_is_max
        thumb_mov_rd_rm(2, 1),                     # MOVS  R2, R1         (R1 is max)
        b_done,                                    # B     done
        # r0_is_max:
        thumb_mov_rd_rm(2, 0),                     # MOVS  R2, R0         (R0 is max)
        # done:
        0xE7FF,                                    # B     .
    ]


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
DEMOS = {
    'arithmetic':    (demo_arithmetic,    "Basic arithmetic (ADD, SUB, CMP)"),
    'loop':          (demo_loop_counter,  "Loop counter (0 to 5)"),
    'fibonacci':     (demo_fibonacci,     "Fibonacci sequence (8 iterations)"),
    'memory':        (demo_memory,        "Memory operations (STR, LDR)"),
    'led_blink':     (demo_led_blink,     "LED blink simulation (GPIO toggle)"),
    'max_value':     (demo_max_value,     "Find maximum of two values"),
}


def main():
    out_dir = os.path.dirname(os.path.abspath(__file__))

    if len(sys.argv) < 2 or sys.argv[1] == 'all':
        print("Generating all demo programs...\n")
        for name, (func, desc) in DEMOS.items():
            prog = func()
            pack(prog, name)
            print(f"    → {desc}\n")
    elif sys.argv[1] in DEMOS:
        name = sys.argv[1]
        func, desc = DEMOS[name]
        print(f"Generating: {name} — {desc}\n")
        prog = func()
        pack(prog, name)
    else:
        print(f"Unknown demo: {sys.argv[1]}")
        print(f"Available: {', '.join(DEMOS.keys())} or 'all'")
        sys.exit(1)

    print(f"\nDone! Binaries saved to: {out_dir}")


if __name__ == '__main__':
    main()
