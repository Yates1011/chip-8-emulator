#include "cpu.hpp"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <cstdlib>
#include <ctime>

/* 
 * =========================
 * CHIP-8 INITIALISATION
 * =========================
 */
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
    chip8.draw_flag = false;

    // Standard CHIP-8 font set (0–F)
    const uint8_t fontset[80] = {
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

    for (int i = 0; i < 80; ++i)
        chip8.memory[i] = fontset[i];

    // Seed random number generator
    std::srand(std::time(nullptr));
}

/* 
 * =========================
 * CPU EMULATION
 * =========================
 */
void emulateCycle(Chip8 &c) {
    c.opcode = (c.memory[c.pc] << 8) | c.memory[c.pc + 1];

    uint8_t x = (c.opcode & 0x0F00) >> 8;
    uint8_t y = (c.opcode & 0x00F0) >> 4;
    uint8_t nn = c.opcode & 0x00FF;
    uint16_t nnn = c.opcode & 0x0FFF;
    uint8_t n = c.opcode & 0x000F;

    switch (static_cast<OpcodeFamily>(c.opcode & 0xF000)) {
        case OpcodeFamily::SYS:
            switch (static_cast<SysOpcode>(nn)) {
                case SysOpcode::CLS:
                    std::memset(c.gfx, 0, sizeof(c.gfx));
                    c.draw_flag = true;
                    c.pc += 2;
                    break;
                case SysOpcode::RET:
                    c.pc = c.stack[--c.sp];
                    break;
                default:
                    c.pc += 2;
                    break;
            }
            break;

        case OpcodeFamily::JP:
            c.pc = nnn;
            break;

        case OpcodeFamily::CALL:
            c.stack[c.sp++] = c.pc + 2;
            c.pc = nnn;
            break;

        case OpcodeFamily::SE_VX: // 3XNN - Skip if Vx == NN
            c.pc += (c.V[x] == nn) ? 4 : 2;
            break;

        case OpcodeFamily::SNE_VX: // 4XNN - Skip if Vx != NN
            c.pc += (c.V[x] != nn) ? 4 : 2;
            break;

        case OpcodeFamily::SE_VY: // 5XY0 - Skip if Vx == Vy
            c.pc += (c.V[x] == c.V[y]) ? 4 : 2;
            break;

        case OpcodeFamily::LD: // 6XNN - Set Vx = NN
            c.V[x] = nn;
            c.pc += 2;
            break;

        case OpcodeFamily::ADD: // 7XNN - Add NN to Vx
            c.V[x] += nn;
            c.pc += 2;
            break;

        case OpcodeFamily::ALU:
            switch (static_cast<AluOpcode>(n)) {
                case AluOpcode::LD: // 8XY0 - Set Vx = Vy
                    c.V[x] = c.V[y];
                    break;

                case AluOpcode::OR: // 8XY1 - Set Vx = Vx OR Vy
                    c.V[x] |= c.V[y];
                    break;

                case AluOpcode::AND: // 8XY2 - Set Vx = Vx AND Vy
                    c.V[x] &= c.V[y];
                    break;

                case AluOpcode::XOR: // 8XY3 - Set Vx = Vx XOR Vy
                    c.V[x] ^= c.V[y];
                    break;

                case AluOpcode::ADD: { // 8XY4 - Add Vy to Vx, set VF = carry
                    uint16_t sum = c.V[x] + c.V[y];
                    c.V[0xF] = sum > 0xFF;
                    c.V[x] = sum & 0xFF;
                    break;
                }

                case AluOpcode::SUB: { // 8XY5 - Set Vx = Vx - Vy, set VF = NOT borrow
                    c.V[0xF] = c.V[x] >= c.V[y];
                    c.V[x] -= c.V[y];
                    break;
                }

                case AluOpcode::SHR: // 8XY6 - Set Vx = Vx >> 1, VF = LSB
                    c.V[0xF] = c.V[x] & 0x01;
                    c.V[x] >>= 1;
                    break;

                case AluOpcode::SUBN: { // 8XY7 - Set Vx = Vy - Vx, set VF = NOT borrow
                    c.V[0xF] = c.V[y] >= c.V[x];
                    c.V[x] = c.V[y] - c.V[x];
                    break;
                }

                case AluOpcode::SHL: // 8XYE - Set Vx = Vx << 1, VF = MSB
                    c.V[0xF] = (c.V[x] & 0x80) >> 7;
                    c.V[x] <<= 1;
                    break;

                default:
                    break;
            }
            c.pc += 2;
            break;

        case OpcodeFamily::SNE: // 9XY0 - Skip if Vx != Vy
            c.pc += (c.V[x] != c.V[y]) ? 4 : 2;
            break;

        case OpcodeFamily::LD_I: // ANNN - Set I = NNN
            c.I = nnn;
            c.pc += 2;
            break;

        case OpcodeFamily::JP_V0: // BNNN - Jump to location NNN + V0
            c.pc = nnn + c.V[0];
            break;

        case OpcodeFamily::RAND: // CXNN - Set Vx = random byte AND NN
            c.V[x] = (std::rand() % 256) & nn;
            c.pc += 2;
            break;

        case OpcodeFamily::DRAW: // DXYN - Draw sprite
            c.V[0xF] = 0;
            for (int row = 0; row < n; ++row) {
                uint8_t sprite = c.memory[c.I + row];
                for (int col = 0; col < 8; ++col) {
                    if (sprite & (0x80 >> col)) {
                        int px = (c.V[x] + col) % SCREEN_WIDTH;
                        int py = (c.V[y] + row) % SCREEN_HEIGHT;
                        if (c.gfx[py][px])
                            c.V[0xF] = 1;
                        c.gfx[py][px] ^= 1;
                    }
                }
            }
            c.draw_flag = true;
            c.pc += 2;
            break;

        case OpcodeFamily::KEY:
            switch (static_cast<KeyOpcode>(nn)) {
                case KeyOpcode::SKP: // EX9E - Skip if key Vx is pressed
                    c.pc += c.keys[c.V[x]] ? 4 : 2;
                    break;

                case KeyOpcode::SKNP: // EXA1 - Skip if key Vx is not pressed
                    c.pc += !c.keys[c.V[x]] ? 4 : 2;
                    break;

                default:
                    c.pc += 2;
                    break;
            }
            break;

        case OpcodeFamily::MISC:
            switch (static_cast<MiscOpcode>(nn)) {
                case MiscOpcode::LD_DT: // FX07 - Set Vx = delay timer
                    c.V[x] = c.delayTimer;
                    break;

                case MiscOpcode::SET_DT: // FX15 - Set delay timer = Vx
                    c.delayTimer = c.V[x];
                    break;

                case MiscOpcode::SET_ST: // FX18 - Set sound timer = Vx
                    c.soundTimer = c.V[x];
                    break;

                case MiscOpcode::ADD_I: // FX1E - Set I = I + Vx
                    c.I += c.V[x];
                    break;

                default:
                    // Handle additional FX opcodes
                    switch (nn) {
                        case 0x0A: { // FX0A - Wait for key press, store in Vx
                            bool key_pressed = false;
                            for (int i = 0; i < NUM_KEYS; ++i) {
                                if (c.keys[i]) {
                                    c.V[x] = i;
                                    key_pressed = true;
                                    break;
                                }
                            }
                            if (!key_pressed)
                                return; // Don't increment PC, wait for key
                            break;
                        }

                        case 0x29: // FX29 - Set I = location of sprite for digit Vx
                            c.I = c.V[x] * 5; // Each font sprite is 5 bytes
                            break;

                        case 0x33: { // FX33 - Store BCD representation of Vx
                            c.memory[c.I] = c.V[x] / 100;
                            c.memory[c.I + 1] = (c.V[x] / 10) % 10;
                            c.memory[c.I + 2] = c.V[x] % 10;
                            break;
                        }

                        case 0x55: // FX55 - Store V0 to Vx in memory starting at I
                            for (int i = 0; i <= x; ++i)
                                c.memory[c.I + i] = c.V[i];
                            break;

                        case 0x65: // FX65 - Read V0 to Vx from memory starting at I
                            for (int i = 0; i <= x; ++i)
                                c.V[i] = c.memory[c.I + i];
                            break;

                        default:
                            break;
                    }
                    break;
            }
            c.pc += 2;
            break;

        default:
            c.pc += 2;
            break;
    }
}

/* 
 * =========================
 * DISPLAY
 * =========================
 */
void printDisplay(Chip8 &chip8) {
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            std::cout << (chip8.gfx[y][x] ? "#" : " ");
        std::cout << "\n";
    }
    std::cout << std::flush;
}

/* 
 * =========================
 * ROM LOADING
 * =========================
 */
bool loadROM(const char *filename, Chip8 &chip8) {
    std::ifstream rom(filename, std::ios::binary | std::ios::ate);
    if (!rom) {
        std::cerr << "Failed to open ROM: " << filename << "\n";
        return false;
    }

    std::streamsize size = rom.tellg();
    if (size <= 0 || size > (CHIP8_RAM - PROGRAM_START)) {
        std::cerr << "Invalid ROM size\n";
        return false;
    }

    rom.seekg(0, std::ios::beg);
    rom.read(reinterpret_cast<char*>(&chip8.memory[PROGRAM_START]), size);
    return true;
}

/* 
 * =========================
 * TERMINAL KEYBOARD INPUT
 * =========================
 */
struct Keyboard {
    termios oldt{};
    bool initialised = false;

    void init() {
        if (initialised) return;
        termios newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
        initialised = true;
    }

    int poll() const {
        return getchar();
    }
};

int mapKey(int c) {
    switch (c) {
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0xC;
        case 'q': return 0x4;
        case 'w': return 0x5;
        case 'e': return 0x6;
        case 'r': return 0xD;
        case 'a': return 0x7;
        case 's': return 0x8;
        case 'd': return 0x9;
        case 'f': return 0xE;
        case 'z': return 0xA;
        case 'x': return 0x0;
        case 'c': return 0xB;
        case 'v': return 0xF;
    }
    return -1;
}

void updateKeys(Chip8 &c, Keyboard &kb) {
    int ch = kb.poll();
    if (ch == EOF) return;
    int key = mapKey(ch);
    if (key >= 0)
        c.keys[key] = 1;
}

/* 
 * =========================
 * MAIN LOOP
 * =========================
 */
int main() {
    Chip8 chip8;
    initialise(chip8);

    if (!loadROM("PONG.ch8", chip8))
        return 1;

    Keyboard keyboard;
    keyboard.init();

    auto lastTimer = std::chrono::high_resolution_clock::now();
    static int keyDecay = 0;

    while (true) {
        updateKeys(chip8, keyboard);
        emulateCycle(chip8);

        keyDecay++;
        if (keyDecay > 6) { // TODO: magic number 
            std::memset(chip8.keys, 0, sizeof(chip8.keys));
            keyDecay = 0;
        }

        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimer).count() >= 16) {
            if (chip8.delayTimer > 0)
                chip8.delayTimer--;
            if (chip8.soundTimer > 0)
                chip8.soundTimer--;
            lastTimer = now;
        }

        if (chip8.draw_flag) {
            system("clear");
            printDisplay(chip8);
            chip8.draw_flag = false;
        }

        usleep(1200);
    }
}