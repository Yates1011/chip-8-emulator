#include "cpu.hpp"
#include <cstdlib>
#include <unistd.h>

void initialise(Chip8 &chip8) {

  chip8.pc = 0x200; // Programs start at 0x200
  chip8.opcode = 0;
  chip8.I = 0;
  chip8.sp = 0;

  std::memset(chip8.memory, 0, sizeof(chip8.memory));
  std::memset(chip8.V, 0, sizeof(chip8.V));
  std::memset(chip8.stack, 0, sizeof(chip8.stack));
  std::memset(chip8.gfx, 0, sizeof(chip8.gfx));
  std::memset(chip8.keys, 0, sizeof(chip8.keys));

  chip8.delayTimer = 0;
  chip8.soundTimer = 0;

  uint8_t fontset[80] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  for (int i = 0; i < 80; i++) {
    chip8.memory[i] = fontset[i];
  }
}

void emulateCycle(Chip8 &chip8) {
  // Fetch
  chip8.opcode =
      chip8.memory[chip8.pc] << 8 |
      chip8.memory[chip8.pc + 1]; // build opcode e.g. 0x61 << 8 | 0x0A = 0x610A

  switch (chip8.opcode & 0xF000) { // e.g. 0x610A & 0xF000 = 0x6000

  case SYS_ADDR:
    switch (chip8.opcode & 0x00FF) {
    case CLS:
      std::memset(chip8.gfx, 0, sizeof(chip8.gfx));
      chip8.pc += 2;
      break;

    case RET:
      chip8.sp--;
      chip8.pc = chip8.stack[chip8.sp];
      break;
    }
    break;

  case JP_ADDR: {
    uint16_t addr = chip8.opcode & 0x0FFF;
    chip8.pc = addr;
    break;
  }

  case LD_VX_BYTE: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t nn = static_cast<uint8_t>(chip8.opcode & 0x00FF);
    chip8.V[x] = nn;
    chip8.pc += 2;
    break;
  }

  case ADD_VX_BYTE: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t nn = static_cast<uint8_t>(chip8.opcode & 0x00FF);
    chip8.V[x] += nn;
    chip8.pc += 2;
    break;
  }

  case LD_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    chip8.V[x] = chip8.V[y];
    chip8.pc += 2;
    break;
  }

  case OR_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    chip8.V[x] |= chip8.V[y];
    chip8.pc += 2;
    break;
  }

  case AND_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    chip8.V[x] &= chip8.V[y];
    chip8.pc += 2;
    break;
  }

  case XOR_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    chip8.V[x] ^= chip8.V[y];
    chip8.pc += 2;
    break;
  }

  case ADD_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    uint16_t sum = chip8.V[x] + chip8.V[y];

    chip8.V[15] = (sum > 0xFF) ? CARRY : NO_CARRY;

    chip8.V[x] = static_cast<uint8_t>(sum & 0xFF);
    chip8.pc += 2;

    break;
  }

  case SUB_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    chip8.V[15] = (chip8.V[x] >= chip8.V[y]) ? NO_BORROW : BORROW;

    chip8.V[x] = chip8.V[x] - chip8.V[y];

    chip8.pc += 2;

    break;
  }

  case SHR_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;

    chip8.V[15] = chip8.V[x] & 1;
    chip8.V[x] >>= 1;

    chip8.pc += 2;

    break;
  }

  case SUBN_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    // chip8.V[x] = chip8.V[y] - chip8.V[x];

    chip8.V[15] = (chip8.V[y] >= chip8.V[x]) ? 1 : 0;

    chip8.V[x] = chip8.V[y] - chip8.V[x];

    chip8.pc += 2;

    break;
  }

  case SHL_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;

    chip8.V[15] = chip8.V[x] & 1;
    chip8.V[x] <<= 1;
    chip8.pc += 2;

    break;
  }

  case SNE_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    if (chip8.V[x] != chip8.V[y])
      chip8.pc += 4;
    else
      chip8.pc += 2;
    break;
  }

  case CALL: {
    chip8.stack[chip8.sp] = chip8.pc + 2;
    chip8.sp++;
    uint16_t addr = chip8.opcode & 0x0FFF;
    chip8.pc = addr;
    break;
  }

  case SE: {
    uint8_t x =
        (chip8.opcode & 0x0F00) >> 8; // TODO doublec check this implementation
    uint8_t nn = static_cast<uint8_t>(chip8.opcode & 0x00FF);

    if (chip8.V[x] == nn)
      chip8.pc += 4;
    else
      chip8.pc += 2;
    break;
  }

  case SNE_VX: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t nn = static_cast<uint8_t>(chip8.opcode & 0x00FF);

    if (chip8.V[x] != nn)
      chip8.pc += 4;
    else
      chip8.pc += 2;
    break;
  }

  case SE_VX_VY: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t y = (chip8.opcode & 0x00F0) >> 4;

    if (chip8.V[x] == chip8.V[y])
      chip8.pc += 4;
    else
      chip8.pc += 2;
    break;
  }

  case LD_I_ADDR: {
    uint16_t nnn = chip8.opcode & 0x0FFF;
    chip8.I = nnn;
    chip8.pc += 2;
    break;
  }

  case JP_BASE: {
    uint16_t nnn = chip8.opcode & 0x0FFF;
    chip8.pc = nnn + chip8.V[0];
    break;
  }

  case RAND_VX: {
    uint8_t x = (chip8.opcode & 0x0F00) >> 8;
    uint8_t nn = static_cast<uint8_t>(chip8.opcode & 0x00FF);

    chip8.V[x] = static_cast<uint8_t>(rand() % 256) & nn;

    chip8.pc += 2;

    break;
  }

  case DRAW_VX: {
    uint8_t n = chip8.opcode & 0x000F;
    uint8_t xReg = (chip8.opcode & 0x0F00) >> 8;
    uint8_t yReg = (chip8.opcode & 0x00F0) >> 4;

    uint8_t xStart = chip8.V[xReg];
    uint8_t yStart = chip8.V[yReg];

    chip8.V[0xF] = 0;

    for (uint8_t row = 0; row < n; row++) {
      uint8_t spriteByte = chip8.memory[chip8.I + row];

      for (uint8_t col = 0; col < COL_WIDTH; col++) {
        uint8_t spriteBit = (spriteByte >> (7 - col)) & 0x1;
        if (spriteBit == 0)
          continue;

        uint8_t x = (xStart + col) % SCREEN_WIDTH;
        uint8_t y = (yStart + row) % SCREEN_HEIGHT;

        // collision detection
        if (chip8.gfx[y][x]) {
          chip8.V[0xF] = 1;
        }

        chip8.gfx[y][x] ^= 1;
      }
    }

    chip8.draw_flag = true;
    chip8.pc += 2;
    break;
  }

  default:
    std::cout << "Unknown opcode: " << std::hex << chip8.opcode << "\n";
    chip8.pc += 2;
  }

  // Timers (60 Hz in real emulator)
  if (chip8.delayTimer > 0)
    chip8.delayTimer--;

  if (chip8.soundTimer > 0)
    chip8.soundTimer--;
}

void printDisplay(Chip8 &chip8) {
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      std::cout << (chip8.gfx[y][x] ? "#" : " ");
    }
    std::cout << "\n";
  }
  std::cout << "\n";
}

bool loadROM(const char *filename, Chip8 &chip8) {
  std::ifstream rom(filename, std::ios::binary | std::ios::ate);
  if (!rom) {
    std::cerr << "Failed to open ROM: " << filename << "\n";
    return false;
  }

  std::streamsize fileSize = rom.tellg();

  if (fileSize <= 0) {
    std::cerr << "Invalid ROM size\n";
    return false;
  }

  const size_t size = static_cast<size_t>(fileSize);

  if (size > (CHIP8_RAM - PROGRAM_START)) { // TODO: maybe just make CHIP_RAM an int...
    std::cerr << "ROM too big\n";
    return false;
  }

  rom.seekg(0, std::ios::beg);

  std::memset(&chip8.memory[PROGRAM_START], 0, CHIP8_RAM - PROGRAM_START);

  // uint8_t and char are layout compatible and read does not care about the
  // type just bytes After reading, chip8.memory contains the exact amount of
  // bytes of the ROM loaded
  if (!rom.read(reinterpret_cast<char *>(&chip8.memory[PROGRAM_START]),
                fileSize)) {
    std::cerr << "Failed to read ROM data\n";
    return false;
  }

  return true;
}

int main() {
  Chip8 chip8;
  initialise(chip8);

  int addr = 0x200;

  // LD V0, 10 (x position)
  chip8.memory[addr++] = 0x60;
  chip8.memory[addr++] = 0x0A;

  // LD V1, 10 (y position)
  chip8.memory[addr++] = 0x61;
  chip8.memory[addr++] = 0x0A;

  // LD I, 0x0014 (address of sprite for "5" in fontset)
  chip8.memory[addr++] = 0xA0;
  chip8.memory[addr++] = 0x19; // 5 * 5 bytes = 25 = 0x19

  // DRW V0, V1, 5 (draw 5 bytes tall sprite)
  chip8.memory[addr++] = 0xD0;
  chip8.memory[addr++] = 0x15;

  // Draw another digit "8" at (20, 10)
  // LD V0, 20
  chip8.memory[addr++] = 0x60;
  chip8.memory[addr++] = 0x14;

  // LD I, 0x0028 (address of sprite for "8")
  chip8.memory[addr++] = 0xA0;
  chip8.memory[addr++] = 0x28; // 8 * 5 = 40 = 0x28

  // DRW V0, V1, 5
  chip8.memory[addr++] = 0xD0;
  chip8.memory[addr++] = 0x15;

  // Infinite loop to keep program running
  // JP 0x20E (jump to current location)
  chip8.memory[addr++] = 0x12;
  chip8.memory[addr++] = 0x0E;

  // Run for a few cycles
  for (int i = 0; i < 2000; i++) {
    emulateCycle(chip8);
  }

  printDisplay(chip8);

  std::cout << "Register values:\n";
  std::cout << "V0: " << std::dec << (int)chip8.V[0] << "\n";
  std::cout << "V1: " << (int)chip8.V[1] << "\n";
  std::cout << "I:  0x" << std::hex << chip8.I << "\n";
  std::cout << "PC: 0x" << std::hex << chip8.pc << "\n";

  initialise(chip8);
  std::cout << "Loading...\n";
  bool loaded = loadROM("PONG.ch8", chip8);

  return 0;
}