#include "cpu.hpp"

typedef enum {
    SYS_ADDR = 0x0000,
    CLS = 0x00E0, 
    RET = 0x00EE, 
    JP_ADDR = 0x1000, 
    LD_VX_BYTE = 0x6000, 
    ADD_VX_BYTE = 0x7000,
    CALL = 0x2000, 
    SE = 0x3000,
    LD_I_ADDR = 0xA000,
    JP_BASE = 0xB000,
} OpcodeType;

void initialise(Chip8& chip8) {
    chip8.pc = 0x200;   // Programs start at 0x200
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

void emulateCycle(Chip8& chip8) {
    // Fetch
    chip8.opcode = chip8.memory[chip8.pc] << 8 |chip8.memory[chip8.pc + 1]; // build opcode e.g. 0x61 << 8 | 0x0A = 0x610A
                   
    switch (chip8.opcode & 0xF000) { // e.g. 0x610A & 0xF000 = 0x6000

    case SYS_ADDR:
        switch (chip8.opcode & 0x00FF) {
        case CLS:
            std::memset(chip8.gfx, 0, sizeof(chip8.gfx));
            chip8.pc += 2;
            break;

        case RET: 
            chip8.pc = chip8.stack[chip8.sp];
            chip8.sp--;
            chip8.pc += 2;  
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
        uint8_t nn = chip8.opcode & 0x00FF;
        chip8.V[x] = nn;
        chip8.pc += 2;
        break;
    }

    case ADD_VX_BYTE: { 
        uint8_t x = (chip8.opcode & 0x0F00) >> 8;
        uint8_t nn = chip8.opcode & 0x00FF;
        chip8.V[x] += nn;
        chip8.pc += 2;
        break;
    }

    case CALL: {
        chip8.sp++;
        chip8.stack[chip8.sp] = chip8.pc;
        uint16_t addr = chip8.opcode & 0x0FFF;
        chip8.pc = addr;
        break;
    }
    
    case SE: {
        uint8_t x = (chip8.opcode & 0x0F00) >> 8;
        uint8_t nn = (chip8.opcode & 0x00FF);

        if (chip8.V[x] == nn)
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

    // TODO: DXYN

    default:
        std::cout << "Unknown opcode: "
                  << std::hex << chip8.opcode << "\n";
        chip8.pc += 2;
    }

    // Timers (60 Hz in real emulator)
    if (chip8.delayTimer > 0)
        chip8.delayTimer--;

    if (chip8.soundTimer > 0)
        chip8.soundTimer--;
}

void printDisplay(Chip8& chip8) {
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            std::cout << (chip8.gfx[x + y * 64] ? "█" : " ");
        }
        std::cout << "\n";
    }
    std::cout << "\n";
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
    for (int i = 0; i < 20; i++) {
        emulateCycle(chip8);
    }

    printDisplay(chip8);
    
    std::cout << "Register values:\n";
    std::cout << "V0: " << std::dec << (int)chip8.V[0] << "\n";
    std::cout << "V1: " << (int)chip8.V[1] << "\n";
    std::cout << "I:  0x" << std::hex << chip8.I << "\n";
    std::cout << "PC: 0x" << std::hex << chip8.pc << "\n";

    // Further implementation would go here

    return 0;
}