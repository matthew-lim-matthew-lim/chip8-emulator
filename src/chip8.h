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

  // Chip8 instructions
  // CLS
  void OP_00E0();

  // RET
  void OP_00EE();

  // JP addr
  void OP_1nnn();

  // CALL addr
  void OP_2nnn();

  // SE Vx, byte
  void OP_3xkk();

  // SNE Vx, byte
  void OP_4xkk();

  // SE Vx, Vy
  void OP_5xy0();

  // LD Vx, byte
  void OP_6xkk();

  // ADD Vx, byte
  void OP_7xkk();

  // LD Vx, Vy
  void OP_8xy0();

  // OR Vx, Vy
  void OP_8xy1();

  // AND Vx, Vy
  void OP_8xy2();

  // XOR Vx, Vy
  void OP_8xy3();

  // ADD Vx, Vy
  void OP_8xy4();

  // SUB Vx, Vy
  void OP_8xy5();

  // SHR Vx {, Vy}
  void OP_8xy6();

  // SUBN Vx, Vy
  void OP_8xy7();

  // SHL Vx {, Vy}
  void OP_8xyE();

  // LD I, addr
  void OP_Annn();

  // JP V0, addr:
  void OP_Bnnn();

  // RND Vx, byte
  void OP_Cxkk();

  // DRW Vx, Vy, nibble
  void Dxyn();

  // SKP Vx
  void Ex9E();

  // SKNP Vx
  void ExA1();

  // LD Vx, DT
  void Fx07();

  // LD Vx, K
  void Fx0A();

  // LD DT, Vx
  void Fx15();

  // LD ST, Vx
  void Fx18();

  // ADD I, Vx
  void Fx1E();
};