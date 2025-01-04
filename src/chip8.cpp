#include "chip8.h"
#include <chrono>
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