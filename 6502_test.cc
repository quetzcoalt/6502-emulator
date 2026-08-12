#include <gtest/gtest.h>
#include "6502.h"

class E6502 : public testing::Test
{
public:

  /* Initializing memory */
  Memory memory;

  /* Initializing CPU */
  CPU cpu;

  virtual void SetUp()
  {
    memory.Initialize();
    cpu.Reset(2, memory);

    /* Point the Reset Vector (0xFFFC/0xFFFD) to the program's start address */
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x80;

    /* Filling up some memory addresses */
    memory[0x0084] = 0x15;
    memory[0x0094] = 0x20;
    memory[0x8415] = 0x25;
    memory[0x8425] = 0x30;  // prev + X
    memory[0x841A] = 0x35;  // prev + Y

    /* Filling up registers */
    cpu.X = 0x10;
    cpu.Y = 0x05;
  }

  virtual void TearDown()
  {

  }
};

TEST_F(E6502, ZeroCycles)
{
  constexpr uint32_t NUM_CYCLES = 0;

  uint32_t cyclesUsed = cpu.Execute(NUM_CYCLES, memory);

  EXPECT_EQ(cyclesUsed, 0);
}

/* -------------------- LDA -------------------- */
TEST_F(E6502, Test_LDA_IM)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_IM, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(2, memory);

  EXPECT_EQ(cpu.A, 0x84);
  EXPECT_EQ(cycles, 2);
}

TEST_F(E6502, Test_LDA_ZP)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ZP, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(3, memory);

  EXPECT_EQ(cpu.A, 0x15);
  EXPECT_EQ(cycles, 3);
}

TEST_F(E6502, Test_LDA_ZPX)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ZPX, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(4, memory);

  EXPECT_EQ(cpu.A, 0x20);
  EXPECT_EQ(cycles, 4);
}

TEST_F(E6502, Test_LDA_ABS)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ABS, 0x15, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(4, memory);

  EXPECT_EQ(cpu.A, 0x25);
  EXPECT_EQ(cycles, 4);
}

TEST_F(E6502, Test_LDA_ABSX)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ABSX, 0x15, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(5, memory);

  EXPECT_EQ(cpu.A, 0x30);
  EXPECT_EQ(cycles, 5);
}

TEST_F(E6502, Test_LDA_ABSY)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ABSY, 0x15, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(5, memory);

  EXPECT_EQ(cpu.A, 0x35);
  EXPECT_EQ(cycles, 5);
}

TEST_F(E6502, TEST_LDA_ZeroStatus)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ABSY, 0x16, 0x87,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(5, memory);

  EXPECT_EQ(cpu.S & 0b00000010, 0b00000010);
  EXPECT_EQ(cycles, 5);
}

TEST_F(E6502, TEST_ADC_0)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_IM, 0xc0,  // 2 
    CPU::INS_TAX,           // 2
    CPU::INS_INX,           // 2
    CPU::INS_ADC_IM, 0xc4,  // 2
    CPU::INS_BRK,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(8, memory);

  EXPECT_EQ(cpu.P, 0b10000001);
  EXPECT_EQ(cpu.A, 0x84);
  EXPECT_EQ(cpu.X, 0xc1);
}

TEST_F(E6502, TEST_ADC_1)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_IM, 0x80,    // 2 
    CPU::INS_STA_ZP, 0x01,    // 3
    CPU::INS_ADC_ZP, 0x01,    // 3
    CPU::INS_BRK,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(8, memory);

  EXPECT_EQ(cpu.P, 0b00000011);
  EXPECT_EQ(cpu.A, 0x00);
  EXPECT_EQ(cpu.S, 0x01ff);
}

TEST_F(E6502, TEST_BNE)
{
  /* Instructions */
  vector<uint32_t> instructions = {
      CPU::INS_LDX_IM, 0x08,        // 2
      CPU::INS_DEX,                 // 2
      CPU::INS_STX_ABS, 0x00, 0x02, // 4
      CPU::INS_CPX_IM, 0x03,        // 2
      CPU::INS_BNE, 0xf8,     // 3
      CPU::INS_STX_ABS, 0x01, 0x02, // 4
      CPU::INS_BRK,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(68, memory);

  EXPECT_EQ(cpu.P, 0b00000011);
  EXPECT_EQ(cpu.X, 0x03);
  EXPECT_EQ(cycles, 62);
}

TEST_F(E6502, Test_INS_JSR)
{
  /* Instructions */
  vector<uint32_t> instructions = {
      CPU::INS_JSR, 0x18, 0x03,   // 6
      CPU::INS_TAX,               // 2
      CPU::INS_TYA,               // 2
      CPU::INS_INY,               // 2
      CPU::INS_BRK,
  };

  /* Other memory locations */
  memory[0x0318] = CPU::INS_LDA_IM;       // 2
  memory[0x0319] = 0x17;
  memory[0x031A] = CPU::INS_LDX_IM;       // 6
  memory[0x031B] = 0x18;
  memory[0x031C] = CPU::INS_LDY_IM;       // 6
  memory[0x031D] = 0x19;
  memory[0x031E] = CPU::INS_RTS;          // 6

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  uint32_t cycles = cpu.Execute(32, memory);

  EXPECT_EQ(cpu.A, 0x19);
}