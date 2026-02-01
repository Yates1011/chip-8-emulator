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
constexpr int COL_WIDTH = 8;
constexpr size_t CHIP8_RAM = 4096;
constexpr size_t PROGRAM_START = 0x200;


struct Chip8 {
  uint8_t memory[MEMORY_SIZE];

  // Registers
  uint8_t V[NUM_REGISTERS]; // V0–VF
  uint16_t I;               // Index register
  uint16_t pc;              // Program counter

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

enum class OpcodeFamily : uint16_t {
  SYS   = 0x0000,
  JP    = 0x1000,
  CALL  = 0x2000,
  SE_VX = 0x3000,
  SNE_VX= 0x4000,
  SE_VY = 0x5000,
  LD    = 0x6000,
  ADD   = 0x7000,
  ALU   = 0x8000,
  SNE   = 0x9000,
  LD_I  = 0xA000,
  JP_V0 = 0xB000,
  RAND  = 0xC000,
  DRAW  = 0xD000,
  KEY   = 0xE000,
  MISC  = 0xF000
};

enum class SysOpcode : uint8_t {
  CLS = 0xE0,
  RET = 0xEE
};

enum class AluOpcode : uint8_t {
  LD   = 0x0,
  OR   = 0x1,
  AND  = 0x2,
  XOR  = 0x3,
  ADD  = 0x4,
  SUB  = 0x5,
  SHR  = 0x6,
  SUBN = 0x7,
  SHL  = 0xE
};

enum class MiscOpcode : uint8_t {
  LD_DT    = 0x07,  // FX07 - Vx = delay timer
  LD_KEY   = 0x0A,  // FX0A - Wait for key press
  SET_DT   = 0x15,  // FX15 - delay timer = Vx
  SET_ST   = 0x18,  // FX18 - sound timer = Vx
  ADD_I    = 0x1E,  // FX1E - I += Vx
  LD_FONT  = 0x29,  // FX29 - I = font sprite for Vx
  LD_BCD   = 0x33,  // FX33 - Store BCD of Vx
  STORE    = 0x55,  // FX55 - Store V0-Vx in memory
  LOAD     = 0x65   // FX65 - Load V0-Vx from memory
};

enum class KeyOpcode : uint8_t {
  SKP  = 0x9E,
  SKNP = 0xA1
};


#endif