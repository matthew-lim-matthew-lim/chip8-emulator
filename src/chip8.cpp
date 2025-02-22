#include "chip8.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sys/types.h>

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

  // Set up function pointer table and subtables.
  // Set main table, which indexes on the first opcode digit.
  table[0x0] = &Chip8::Table0;
  table[0x1] = &Chip8::OP_1nnn;
  table[0x2] = &Chip8::OP_2nnn;
  table[0x3] = &Chip8::OP_3xkk;
  table[0x4] = &Chip8::OP_4xkk;
  table[0x5] = &Chip8::OP_5xy0;
  table[0x6] = &Chip8::OP_6xkk;
  table[0x7] = &Chip8::OP_7xkk;
  table[0x8] = &Chip8::Table8;
  table[0x9] = &Chip8::OP_9xy0;
  table[0xA] = &Chip8::OP_Annn;
  table[0xB] = &Chip8::OP_Bnnn;
  table[0xC] = &Chip8::OP_Cxkk;
  table[0xD] = &Chip8::OP_Dxyn;
  table[0xE] = &Chip8::TableE;
  table[0xF] = &Chip8::TableF;

  // Set default pointer of subtables to OP_NULL.
  for (size_t i = 0; i <= 0xE; i++) {
    table0[i] = &Chip8::OP_NULL;
    table8[i] = &Chip8::OP_NULL;
    tableE[i] = &Chip8::OP_NULL;
  }

  // Set subtables which index on the remainder of the opcode.
  table0[0x0] = &Chip8::OP_00E0;
  table0[0xE] = &Chip8::OP_00EE;

  table8[0x0] = &Chip8::OP_8xy0;
  table8[0x1] = &Chip8::OP_8xy1;
  table8[0x2] = &Chip8::OP_8xy2;
  table8[0x3] = &Chip8::OP_8xy3;
  table8[0x4] = &Chip8::OP_8xy4;
  table8[0x5] = &Chip8::OP_8xy5;
  table8[0x6] = &Chip8::OP_8xy6;
  table8[0x7] = &Chip8::OP_8xy7;
  table8[0xE] = &Chip8::OP_8xyE;

  tableE[0x1] = &Chip8::OP_ExA1;
  tableE[0xE] = &Chip8::OP_Ex9E;

  for (size_t i = 0; i <= 0x65; i++) {
    tableF[i] = &Chip8::OP_NULL;
  }

  tableF[0x07] = &Chip8::OP_Fx07;
  tableF[0x0A] = &Chip8::OP_Fx0A;
  tableF[0x15] = &Chip8::OP_Fx15;
  tableF[0x18] = &Chip8::OP_Fx18;
  tableF[0x1E] = &Chip8::OP_Fx1E;
  tableF[0x29] = &Chip8::OP_Fx29;
  tableF[0x33] = &Chip8::OP_Fx33;
  tableF[0x55] = &Chip8::OP_Fx55;
  tableF[0x65] = &Chip8::OP_Fx65;
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

    std::cout << "ROM loaded, size: " << size << " bytes\n";

    // Free the buffer
    delete[] buffer;
  }
}

// Decode Opcode instruction using Function pointer table

void Chip8::Table0() { ((*this).*(table0[opcode & 0x000Fu]))(); }

void Chip8::Table8() { ((*this).*(table8[opcode & 0x000Fu]))(); }

void Chip8::TableE() { ((*this).*(tableE[opcode & 0x000Fu]))(); }

void Chip8::TableF() { ((*this).*(tableF[opcode & 0x00FFu]))(); }

// Chip8 Cycle
void Chip8::Cycle() {
  // Fetch next instruction
  opcode = (memory[pc] << 8) | memory[pc + 1];

  // Increment PC before we execute
  pc += 2;

  // Decode the instruction to determine what operation needs to occur
  uint8_t opcodeIndex1 = (opcode & 0xF000u) >> 12;

  // Find the corresponding table and then execute.
  ((*this).*(table[opcodeIndex1]))();

  // Decrement delay timer
  if (delayTimer > 0) {
    --delayTimer;
  }

  // Decrement sound time
  if (soundTimer > 0) {
    --soundTimer;
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
  registers[x] = registers[y];
}

// OR Vx, Vy: Set Vx = Vx OR Vy.
void Chip8::OP_8xy1() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t y = (opcode & 0x00F0u) >> 4;
  registers[x] = registers[x] | registers[y];
}

// AND Vx, Vy: Set Vx = Vx AND Vy.
void Chip8::OP_8xy2() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t y = (opcode & 0x00F0u) >> 4;
  registers[x] = registers[x] & registers[y];
}

// XOR Vx, Vy: Set Vx = Vx XOR Vy.
void Chip8::OP_8xy3() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t y = (opcode & 0x00F0u) >> 4;
  registers[x] = registers[x] ^ registers[y];
}

// ADD Vx, Vy: Set Vx = Vx + Vy, set VF = carry.
void Chip8::OP_8xy4() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t y = (opcode & 0x00F0u) >> 4;
  uint16_t sum = registers[x] + registers[y];
  registers[x] = sum & 0x00FFu; // Mask to get the bottom 8 bits.
  sum = sum >> 8;
  registers[15] = sum;
}

// SUB Vx, Vy: Set Vx = Vx - Vy, set VF = NOT borrow.
void Chip8::OP_8xy5() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t y = (opcode & 0x00F0u) >> 4;
  if (registers[x] < registers[y]) {
    registers[15] = 0;
  } else {
    registers[15] = 1;
  }
  // Wrap around automatically happens with unsighted values, so simply subtract
  // Vy from Vx!
  registers[x] = registers[x] - registers[y];
}

// SHR Vx {, Vy}: Set Vx = Vx SHR 1.
void Chip8::OP_8xy6() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  // Set VF to the least significant bit of Vx
  registers[15] = registers[x] & 1;
  // Divide Vx by 2 by bit-shifting right.
  registers[x] = registers[x] >> 1;
}

// SUBN Vx, Vy: Set Vx = Vy - Vx, set VF = NOT borrow.
void Chip8::OP_8xy7() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t y = (opcode & 0x00F0u) >> 4;
  if (registers[y] < registers[x]) {
    registers[15] = 0;
  } else {
    registers[15] = 1;
  }
  registers[x] = registers[y] - registers[x];
}

// SHL Vx {, Vy}: Set Vx = Vx SHL 1.
void Chip8::OP_8xyE() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  registers[15] = (registers[x] & 0x80u) >> 7;
  registers[x] = registers[x] << 1;
}

// SNE Vx, Vy: Skip next instruction if Vx != Vy.
void Chip8::OP_9xy0() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];
  uint8_t y = (opcode & 0x00F0u) >> 4;
  uint8_t Vy = registers[y];
  if (Vx != Vy) {
    pc += 2;
  }
}

// LD I, addr: Set I = nnn.
void Chip8::OP_Annn() {
  uint16_t nnn = opcode & 0x0FFFu;
  // 'I' refers to the index register.
  index = nnn;
}

// JP V0, addr: Jump to location nnn + V0.
void Chip8::OP_Bnnn() {
  uint16_t nnn = opcode & 0x0FFFu;
  pc = nnn + registers[0];
}

// RND Vx, byte: Set Vx = random byte AND kk.
void Chip8::OP_Cxkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t rand = randByte(randGen);
  uint8_t kk = opcode & 0x00FFu;
  registers[x] = rand & kk;
}

// DRW Vx, Vy, nibble: Display n-byte sprite starting at memory location I at
// (Vx, Vy), set VF = collision.
void Chip8::OP_Dxyn() {
  // n is the height of the sprite in pixels.
  // The sprite is 8 pixels wide. This works since Chip8 sprites are always 8
  // pixels wide.
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];
  uint8_t y = (opcode & 0x00F0u) >> 4;
  uint8_t Vy = registers[y];
  uint8_t n = opcode & 0x000Fu;

  // Read 'n' bytes from 'index' and write them.
  registers[15] = 0;
  for (int i = 0; i < n; ++i) {
    // Dont need to modulo when we read 'n' bytes from memory.
    uint8_t spriteByte = memory[index + i];

    for (int j = 0; j < 8; ++j) {
      // Wrapping in X and Y for the sprite.
      uint8_t pixelX = (Vx + j) % 64;
      uint8_t pixelY = (Vy + i) % 32;

      uint32_t spritePixel = (spriteByte >> (7 - j)) & 1;

      uint32_t &oldPixel = video[pixelY * 64 + pixelX];

      // Check collision: if oldPixel is white (0xFFFFFFFF) and we're about to
      // draw a "1"
      if (spritePixel && (oldPixel == 0xFFFFFFFF)) {
        registers[0xF] = 1;
      }

      // Toggle: if it was white, turn it black; if black, turn it white
      if (spritePixel) {
        oldPixel = (oldPixel == 0xFFFFFFFF) ? 0xFF000000 : 0xFFFFFFFF;
      }
    }
  }
}

// SKP Vx: Skip next instruction if key with the value of Vx is pressed.
void Chip8::OP_Ex9E() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];
  if (keypad[Vx]) {
    pc += 2;
  }
}

// SKNP Vx: Skip next instruction if key with the value of Vx is not pressed.
void Chip8::OP_ExA1() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];
  if (!keypad[Vx]) {
    pc += 2;
  }
}

// LD Vx, DT: Set Vx = delay timer value.
void Chip8::OP_Fx07() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  registers[x] = delayTimer;
}

// LD Vx, K: Wait for a key press, store the value of the key in Vx.
void Chip8::OP_Fx0A() {
  uint8_t x = (opcode & 0x0F00u) >> 8;

  // 'wait' by decrementing the PC by 2 whenever a keypad value is not detected.
  // This makes the same instruction run repeatedly (which has wait behaviour).
  if (keypad[0]) {
    registers[x] = 0;
  } else if (keypad[1]) {
    registers[x] = 1;
  } else if (keypad[2]) {
    registers[x] = 2;
  } else if (keypad[3]) {
    registers[x] = 3;
  } else if (keypad[4]) {
    registers[x] = 4;
  } else if (keypad[5]) {
    registers[x] = 5;
  } else if (keypad[6]) {
    registers[x] = 6;
  } else if (keypad[7]) {
    registers[x] = 7;
  } else if (keypad[8]) {
    registers[x] = 8;
  } else if (keypad[9]) {
    registers[x] = 9;
  } else if (keypad[10]) {
    registers[x] = 10;
  } else if (keypad[11]) {
    registers[x] = 11;
  } else if (keypad[12]) {
    registers[x] = 12;
  } else if (keypad[13]) {
    registers[x] = 13;
  } else if (keypad[14]) {
    registers[x] = 14;
  } else if (keypad[15]) {
    registers[x] = 15;
  } else {
    pc -= 2;
  }
}

// LD DT, Vx: Set delay timer = Vx.
void Chip8::OP_Fx15() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];

  delayTimer = Vx;
}

// LD ST, Vx: Set sound timer = Vx.
void Chip8::OP_Fx18() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];

  soundTimer = Vx;
}

// ADD I, Vx: Set I = I + Vx.
void Chip8::OP_Fx1E() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];

  index = index + Vx;
}

// LD F, Vx: Set I = location of sprite for digit Vx.
void Chip8::OP_Fx29() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];

  index = Vx * 5 + FONTSET_START_ADDRESS;
}

// LD B, Vx: Store BCD representation of Vx in memory locations I, I+1, and I+2.
void Chip8::OP_Fx33() {
  uint8_t x = (opcode & 0x0F00u) >> 8;
  uint8_t Vx = registers[x];
  memory[index] = Vx / 100;
  memory[index + 1] = Vx / 10 % 10;
  memory[index + 2] = Vx % 10;
}

// LD [I], Vx: Store registers V0 through Vx in memory starting at location I.
void Chip8::OP_Fx55() {
  uint8_t x = (opcode & 0x0F00u) >> 8;

  for (int i = 0; i <= x; ++i) {
    memory[index + i] = registers[i];
  }
}

// LD Vx, [I]
void Chip8::OP_Fx65() {
  uint8_t x = (opcode & 0x0F00u) >> 8;

  for (int i = 0; i <= x; ++i) {
    registers[i] = memory[index + i];
  }
}

// Do nothing - Default function table function.
void Chip8::OP_NULL() {}