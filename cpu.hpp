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


    bool gfx[SCREEN_WIDTH * SCREEN_HEIGHT];

    // Keypad
    bool keys[NUM_KEYS];

    // Current opcode
    uint16_t opcode;
};
