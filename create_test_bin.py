#!/usr/bin/env python3
"""
Скрипт для создания тестовых .bin файлов для STM32 симулятора.
Генерирует Thumb-инструкции в little-endian формате.
"""

import struct
import sys

def write_bin(filename, instructions):
    """Записывает список инструкций в бинарный файл."""
    with open(filename, "wb") as f:
        for instr in instructions:
            f.write(struct.pack("<H", instr))
    print(f"[✓] Created {filename} ({len(instructions)} instructions, {len(instructions)*2} bytes)")

def create_demo():
    """Демо-программа: сложение двух чисел."""
    instructions = [
        0x2005,  # MOV R0, #5
        0x2103,  # MOV R1, #3
        0x1882,  # ADD R2, R0, R1  → R2 = 8
        0xE7FE,  # B . (бесконечный цикл)
    ]
    write_bin("demo.bin", instructions)
    print("    Expected: R0=5, R1=3, R2=8")

def create_loop():
    """Программа с циклом."""
    instructions = [
        0x2000,  # MOV R0, #0      (счётчик)
        0x210A,  # MOV R1, #10     (граница)
        # Loop:
        0x1C40,  # ADDS R0, R0, #1
        0x4288,  # CMP R0, R1
        0xDBFB,  # BLT Loop (offset -5)
        0xE7FE,  # B . (цикл)
    ]
    write_bin("loop.bin", instructions)
    print("    Expected: R0=10 (после 10 итераций)")

def create_memory_test():
    """Тест доступа к памяти."""
    instructions = [
        0x2000,  # MOV R0, #0      (адрес в SRAM: 0x20000000)
        0x2142,  # MOV R1, #0x42   (значение)
        0x6001,  # STR R1, [R0]    (запись по адресу 0x20000000)
        0x6802,  # LDR R2, [R0]    (чтение)
        0xE7FE,  # B .
    ]
    write_bin("memory.bin", instructions)
    print("    Expected: R1=0x42, R2=0x42 (запись/чтение)")

def create_all():
    """Создать все тестовые файлы."""
    print("Creating test binary files...\n")
    create_demo()
    create_loop()
    create_memory_test()
    print("\n[✓] All test files created!")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        command = sys.argv[1].lower()
        if command == "demo":
            create_demo()
        elif command == "loop":
            create_loop()
        elif command == "memory":
            create_memory_test()
        elif command == "all":
            create_all()
        else:
            print(f"Unknown command: {command}")
            print("Usage: python create_test_bin.py [demo|loop|memory|all]")
            sys.exit(1)
    else:
        create_all()
