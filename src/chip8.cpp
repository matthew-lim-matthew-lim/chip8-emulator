#include "chip8.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>

// Chip8 start address is 0x200 for instructions from the ROM
const unsigned int START_ADDRESS = 0x200;

// Each character of the Chip8 fontset is a 5 byte sprite.
// The memory 0x050-0x0A0 is reserved for the 16 built-in characters (0-F)
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;

uint8_t fontset[FONTSET_SIZE] = {
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

Chip8::Chip8()
    // Initialise the random number generator with the current time
    : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
  // Initialise program counter to the start address
  pc = START_ADDRESS;

  // Load fonts into memory
  for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
    memory[FONTSET_START_ADDRESS + i] = fontset[i];
  }

  // Initialise distribution which we will use with the random number generator
  // using randByte(randGen)
  randByte = std::uniform_int_distribution<uint8_t>(0, 255U);
}

void Chip8::LoadROM(char const *filename) {
  // Open the file as a stream of binary and move the file pointer to the end
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (file.is_open()) {
    // Get size of file and allocate a buffer to hold the contents
    std::streampos size = file.tellg();
    char *buffer = new char[size];

    // Go back to the beginning of the file and fill the buffer with the file
    // contents
    file.seekg(0, std::ios::beg);
    file.read(buffer, size);
    file.close();

    // Load the ROM contents (from the buffer) into the Chip8's memory, starting
    // at 0x200
    for (long i = 0; i < size; ++i) {
      memory[START_ADDRESS + i] = buffer[i];
    }

    // Free the buffer
    delete[] buffer;
  }
}

// Chip8 instructions

// CLS: Clear the display
void Chip8::OP_00E0() {
  for (int y = 0; y < 32; ++y) {
    for (int x = 0; x < 64; ++x) {
      video[y * 64 + x] = 0;
    }
  }
}

// RET: Return from a subroutine
void Chip8::OP_00EE() {
  // Decrement SP
  --sp;
  // Set PC to top of stack
  pc = stack[sp];
}

// JP addr: Jump to location nnn.
void Chip8::OP_1nnn() {
  // Mask the opcode with 0x0FFFu to get the 'nnn' address (which is being
  // jumped to)
  uint16_t address = opcode & 0x0FFFu;
  pc = address;
}

// CALL addr: Call subroutine at nnn.
void Chip8::OP_2nnn() {
  // Current PC should point to next instruction. This is because return should
  // give us the next instruction, not the same call instruction (that would
  // result in infinite loop).

  // Set top of stack to PC
  stack[sp] = pc;
  // Increment SP
  ++sp;

  uint16_t address = opcode & 0x0FFFu;
  pc = address;
}

// SE Vx, byte: Skip next instruction if Vx = kk.
void Chip8::OP_3xkk() {
  // Get the register index and the value at the index
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];
  // Compare it with kk
  uint8_t kk = opcode & 0x00FFu;
  if (Vx == kk) {
    // Increment program counter by 2 bytes (each instruction is 2 bytes)
    pc += 2;
  }
}

// SNE Vx, byte: Skip next instruction if Vx != kk.
void Chip8::OP_4xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];
  uint8_t kk = opcode & 0x00FFu;
  if (Vx != kk) {
    pc += 2;
  }
}

// SE Vx, Vy: Skip next instruction if Vx = Vy.
void Chip8::OP_5xy0() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];
  uint8_t y = (opcode & 0x00F0u) >> 4;
  uint8_t Vy = registers[y];
  if (Vx == Vy) {
    pc += 2;
  }
}

// LD Vx, byte: Set Vx = kk.
void Chip8::OP_6xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t kk = opcode & 0x00FFu;
  registers[x] = kk;
}

// ADD Vx, byte: Set Vx = Vx + kk.
void Chip8::OP_7xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t kk = opcode & 0x00FFu;
  registers[x] += kk;
}

// LD Vx, Vy: Set Vx = Vy.
void Chip8::OP_8xy0() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t y = (opcode & 0x00F0u) >> 4;
  uint8_t Vy = registers[y];
  registers[x] = Vy;
}