#ifndef CPU_HPP
#define CPU_HPP

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

constexpr int NUM_REGISTERS = 16;
constexpr int MEMORY_SIZE = 4096;
constexpr int START_ADDRESS = 0x200;
constexpr int SCREEN_WIDTH = 64;
constexpr int SCREEN_HEIGHT = 32;
constexpr int NUM_KEYS = 16;
constexpr int CARRY = 1;
constexpr int NO_CARRY = 0;
constexpr int NO_BORROW = 1;
constexpr int BORROW = 0;
constexpr int NUM_COLS = 8;


enum OpcodeType {
    SYS_ADDR = 0x0000,
    CLS = 0x00E0, 
    RET = 0x00EE, 
    JP_ADDR = 0x1000, 
    CALL = 0x2000,
    SE = 0x3000,
    SNE_VX = 0x4000,
    SE_VX_VY = 0x5000,
    LD_VX_BYTE = 0x6000, 
    ADD_VX_BYTE = 0x7000,
    LD_VX_VY = 0x8000,
    OR_VX_VY = 0x8001,
    AND_VX_VY = 0x8002, 
    XOR_VX_VY = 0x8003,
    ADD_VX_VY = 0x8004, 
    SUB_VX_VY = 0x8005, 
    SHR_VX_VY = 0x8006,
    SUBN_VX_VY = 0x8007,
    SHL_VX_VY = 0x800E,
    SNE_VX_VY = 0x9000,
    LD_I_ADDR = 0xA000,
    JP_BASE = 0xB000,
    RAND_VX = 0xC000,
    DRAW_VX = 0xD000,
};

struct Chip8 {
    uint8_t memory[MEMORY_SIZE];

    // Registers
    uint8_t V[NUM_REGISTERS];      // V0–VF
    uint16_t I;         // Index register
    uint16_t pc;        // Program counter

    // Stack
    uint16_t stack[16];
    uint8_t sp;

    uint8_t delayTimer;
    uint8_t soundTimer;


    // bool gfx[SCREEN_WIDTH * SCREEN_HEIGHT];
    bool gfx[SCREEN_HEIGHT][SCREEN_WIDTH];

    // Keypad
    bool keys[NUM_KEYS];

    // Current opcode
    uint16_t opcode;

    bool draw_flag;
};

#endif