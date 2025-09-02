#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <stack>
#include <streambuf>
#include <string>
#include <vector>
#include "chip8.h"

constexpr uint8_t width = 64;
constexpr uint8_t length = 32;
constexpr uint16_t programEntry = 0x200;

// *****************************************
// Memory **********************************
// *****************************************
Memory::Memory() : ram(4096, 0), stack{}, delayTimer(0), soundTimer(0), display(width * length, 0) {
    const std::vector<uint8_t> fontData = {
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

    for (uint8_t i = 0; i < fontData.size(); ++i) {
        ram[i] = fontData[i];
    }
}

void Memory::loadRom(std::string fileName) {
    std::ifstream rom(fileName, std::ios::binary | std::ios::ate);
    if (!rom.is_open()) {
        std::cerr << "Failed to open: " << fileName << std::endl;
        std::cerr << "Current working directory: " << std::filesystem::current_path() << std::endl;
    }

    std::streamsize romSize = rom.tellg();
    rom.seekg(0, std::ios::beg);
    std::vector<char> buffer(romSize);

    rom.read(buffer.data(), romSize);

    for (uint16_t i = 0; i < romSize; ++i) {
        ram[programEntry + i] = static_cast<uint8_t>(buffer[i]);
    }
}

// *****************************************
// CPU *************************************
// *****************************************
CPU::CPU() : pc(0x200), iReg(0), v(16, 0), lastKeyReleased(-1), keyState(16, false) {}

uint16_t CPU::fetch(Memory& mem) {
    uint16_t msb = mem.ram[pc] << 8;
    uint16_t lsb = mem.ram[pc + 1];
    uint16_t instruction = msb | lsb;

    pc += 2;

    std::cout << "Executing opcode " << std::hex << instruction << "\r\n";
    return instruction;
}

void CPU::decodeAndExecute(Memory& mem, sf::RenderWindow& window, uint16_t opcode) {
    uint16_t type = (opcode >> 12) & 0x0F;
    uint16_t x = (opcode >> 8) & 0x0F;
    uint16_t y = (opcode >> 4) & 0x0F;
    uint16_t n = opcode & 0x0F;
    uint16_t nn = (y << 4) | n;
    uint16_t nnn = (x << 8) | nn;

    switch (type) {
    case (0x00):
        decode0(mem, nnn);
        break;

    case (0x01):
        pc = nnn;
        break;

    case (0x02):
        mem.stack.push(pc);
        pc = nnn;
        break;

    case (0x03):
        if (v[x] == nn) pc += 2;
        break;

    case (0x04):
        if (v[x] != nn) pc += 2;
        break;

    case (0x05):
        if (v[x] == v[y]) pc += 2;
        break;

    case (0x06):
        v[x] = nn;
        break;

    case (0x07):
        v[x] += nn;
        break;

    case (0x08):
        decode8(x, y, n);
        break;

    case (0x09):
        if (v[x] != v[y]) pc += 2;
        break;

    case (0x0A):
        iReg = nnn;
        break;

    case (0x0B):
        pc = v[0] + nnn;        // TODO: Make this configurable
        break;

    case (0x0C):
        v[x] = static_cast<uint8_t>(rand()) & nn;
        break;

    case (0x0D):
        drawGraphics(mem, x, y, n);
        break;

    case (0x0E):
        decodeE(x, nn);
        break;

    case (0x0F):
        decodeF(mem, x, nn);
        break;

    default:
        break;
    }
}

void CPU::decode0(Memory& mem, uint16_t nnn) {
    switch (nnn) {
    case (0x0E0):
        for (uint8_t i = 0; i < length; ++i) {
            for (uint8_t j = 0; j < width; ++j) {
                uint16_t index = i * width + j;
                mem.display[index] = 0;
            }
        }
        break;

    case (0x0EE):
        pc = mem.stack.top();
        mem.stack.pop();
        break;

    default:
        break;
    }
}

void CPU::decode8(uint16_t x, uint16_t y, uint16_t n) {
    uint8_t prev = v[x];

    switch (n) {
    case (0x00):
        v[x] = v[y];
        break;

    case (0x01):
        v[x] |= v[y];
        v[0x0F] = 0;
        break;

    case (0x02):
        v[x] &= v[y];
        v[0x0F] = 0;
        break;

    case (0x03):
        v[x] ^= v[y];
        v[0x0F] = 0;
        break;

    case (0x04):
        v[x] += v[y];
        v[0x0F] = (v[x] < prev) ? 1 : 0;
        break;

    case (0x05):
        v[x] -= v[y];
        v[0x0F] = (v[x] < prev) ? 1 : 0;
        break;

    case (0x06):
        v[x] = v[y] >> 1;
        v[0x0F] = v[y] & static_cast<uint8_t>(1);
        /*v[x] >>= 1;
        v[0x0F] = prev & static_cast<uint8_t>(1);*/
        break;

    case (0x07):
        v[x] = v[y] - v[x];
        v[0x0F] = (v[x] < v[y]) ? 1 : 0;
        break;

    case (0x0E):
        v[x] = v[y] << 1;       
        v[0x0F] = (v[y] & static_cast<uint8_t>(128)) >> 7;
        /*v[x] <<= 1;
        v[0x0F] = (prev & static_cast<uint8_t>(128)) >> 7;*/
        break;

    default:
        std::cerr << "Unknown instruction!" << "\r\n";
        break;
    }
}

void CPU::drawGraphics(Memory& mem, uint16_t x, uint16_t y, uint16_t n) {
    uint8_t xCoord = v[x] % width;
    uint8_t yCoord = v[y] % length;
    v[0x0F] = 0;

    for (uint8_t i = 0; i < n; ++i) {
        uint8_t sprite = mem.ram[iReg + i];

        for (int8_t j = 7; j >= 0; --j) {
            uint16_t index = (yCoord * width) + xCoord;
            uint8_t prev = mem.display[index];
            mem.display[index] ^= (sprite >> j) & static_cast<uint8_t>(1);
            uint8_t curr = mem.display[index];
            if (prev && !curr) v[0x0F] = 1;

            ++xCoord;
            if (xCoord > 63) break;
        }

        xCoord = v[x] & 63;
        ++yCoord;
        if (yCoord > 31) break;
    }
}

void CPU::decodeE(uint16_t x, uint16_t nn) {
    bool keyPressed = keyState[v[x]];

    switch (nn) {
    case (0x9E):
        if (keyPressed) pc += 2;
        break;
        
    case (0xA1):
        if (!keyPressed) pc += 2;
        break;

    default:
        break;
    }
}

void CPU::decodeF(Memory& mem, uint16_t x, uint16_t nn) {
    uint16_t prev = iReg;
    
    switch (nn) {
    case (0x07):
        v[x] = mem.delayTimer;
        break;

    case (0x15):
        mem.delayTimer = v[x];
        break;

    case (0x18):
        mem.soundTimer = v[x];
        break;

    case (0x1E):
        iReg += v[x];
        if (iReg < prev) v[0x0F] = 1;
        else v[0x0F] = 0;
        break;

    case (0x0A):
        if (lastKeyReleased == -1) {
            pc -= 2;
            break;
        }
        v[x] = lastKeyReleased;
        break;

    case (0x29):
        iReg = v[x] * 5;
        break;

    case (0x33):
        mem.ram[iReg] = (v[x] / 100) % 10;
        mem.ram[iReg + 1] = (v[x] / 10) % 10;
        mem.ram[iReg + 2] = v[x] % 10;
        break;

    case (0x55):
        for (uint8_t i = 0; i <= x; ++i) {
            mem.ram[iReg] = v[i];
            ++iReg;
            // mem.ram[iReg + i] = v[i];
        }
        break;

    case (0x65):
        for (uint8_t i = 0; i <= x; ++i) {
            v[i] = mem.ram[iReg];
            ++iReg;
            // v[i] = mem.ram[iReg + i];
        }
        break;

    default:
        break;
    }
}