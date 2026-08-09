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

/* -------------------- LDA -------------------- */
TEST_F(E6502, Test_LDA_IM)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_IM, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  cpu.Execute(2, memory);

  EXPECT_EQ(cpu.A, 0x84);
}

TEST_F(E6502, Test_LDA_ZP)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ZP, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  cpu.Execute(3, memory);

  EXPECT_EQ(cpu.A, 0x15);
}

TEST_F(E6502, Test_LDA_ZPX)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ZPX, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  cpu.Execute(4, memory);

  EXPECT_EQ(cpu.A, 0x20);
}

TEST_F(E6502, Test_LDA_ABS)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ABS, 0x15, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  cpu.Execute(4, memory);

  EXPECT_EQ(cpu.A, 0x25);
}

TEST_F(E6502, Test_LDA_ABSX)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ABSX, 0x15, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  cpu.Execute(5, memory);

  EXPECT_EQ(cpu.A, 0x30);
}

TEST_F(E6502, Test_LDA_ABSY)
{
  vector<uint32_t> instructions = {
    CPU::INS_LDA_ABSY, 0x15, 0x84,
  };

  cpu.MountProgram(instructions, memory, instructions.size());

  /* Execution */
  cpu.Execute(5, memory);

  EXPECT_EQ(cpu.A, 0x35);
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
  cpu.Execute(32, memory);

  EXPECT_EQ(cpu.A, 0x19);
}