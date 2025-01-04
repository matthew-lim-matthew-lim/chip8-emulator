#include <cstdint>
#include <random>

class Chip8 {
public:
  Chip8();                            // Constructor
  void LoadROM(char const *filename); // Load a ROM into memory

private:
  // Chip8 class members - Components of Chip8
  uint8_t registers[16]{};
  uint8_t memory[4096]{};
  uint16_t index{};
  uint16_t pc{};
  uint16_t stack[16]{};
  uint8_t sp{};
  uint8_t delayTimer{};
  uint8_t soundTimer{};
  uint8_t keypad[16]{};
  uint32_t video[64 * 32]{};
  uint16_t opcode;

  // Random number generation. Used for Cxkk instruction
  std::default_random_engine randGen;
  std::uniform_int_distribution<uint8_t> randByte;
};