#include "6502.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <random>
#include <format>

#include <fstream>

struct CPUTestCase
{
  uint8_t instruction;
  nlohmann::json test;
};

class E6502 : public testing::TestWithParam<CPUTestCase>
{
public:

  /* Initializing memory */
  Memory memory;

  /* Initializing CPU */
  CPU cpu;

  void SetUp() override
  {
      memory.Initialize();
      cpu.Reset(2, memory);
  }
};

TEST_P(E6502, AllTests)
{
  const auto& test = GetParam().test;

  memory.Initialize();
  cpu.Reset(2, memory);

  /* Setting up registers */
  cpu.PC = test["initial"]["pc"];
  cpu.S = test["initial"]["s"];
  cpu.A = test["initial"]["a"];
  cpu.X = test["initial"]["x"];
  cpu.Y = test["initial"]["y"];
  cpu.P = test["initial"]["p"];

  /* Setting up memory addresses */
  for (auto mem : test["initial"]["ram"])
  {
    memory[mem[0]] = mem[1];
  }

  /* Execution */
  uint32_t cycles = cpu.Execute(test["cycles"].size(), memory);

  EXPECT_EQ(test["final"]["pc"], cpu.PC);
  EXPECT_EQ(test["final"]["s"], cpu.S);
  EXPECT_EQ(test["final"]["a"], cpu.A);
  EXPECT_EQ(test["final"]["x"], cpu.X);
  EXPECT_EQ(test["final"]["y"], cpu.Y);
  EXPECT_EQ(test["final"]["p"], cpu.P);
}

std::vector<CPUTestCase> LoadTestCases(std::initializer_list<uint8_t> instructions)
{
  std::vector<CPUTestCase> result;

  for (uint8_t instruction : instructions)
  {
    std::ifstream file(
      format("tests/entries/{:02x}.json", instruction)
    );

    if (!file.is_open())
    {
      throw std::runtime_error(format("Could not open {:x}.json", instruction));
    }

    nlohmann::json tests;
    file >> tests;

    size_t count = 0;

    for (const auto& test : tests)
    {
        result.push_back({
            .instruction = instruction,
            .test = test
        });

        if (++count >= 30)
            break;
    }
  }

  return result;
}

INSTANTIATE_TEST_SUITE_P(
    JSONTests,
    E6502,
    testing::ValuesIn(LoadTestCases({
      // CPU::INS_LDA_IM,
      // CPU::INS_LDA_ZP,
      // CPU::INS_LDA_ZPX,
      // CPU::INS_LDA_ABS,
      // CPU::INS_LDA_ABSX,
      // CPU::INS_LDA_ABSY,
      // CPU::INS_LDA_IDX,
      // CPU::INS_LDA_IDY,

      // CPU::INS_LDX_IM,
      // CPU::INS_LDX_ZP,
      // CPU::INS_LDX_ZPY,
      // CPU::INS_LDX_ABS,
      // CPU::INS_LDX_ABSY,

      // CPU::INS_LDY_IM,
      // CPU::INS_LDY_ZP,
      // CPU::INS_LDY_ZPX,
      // CPU::INS_LDY_ABS,
      // CPU::INS_LDY_ABSX,

      //  CPU::INS_STA_ZP,
      //  CPU::INS_STA_ZPX,
      //  CPU::INS_STA_ABS,
      //  CPU::INS_STA_ABSX,
      //  CPU::INS_STA_ABSY,
      //  CPU::INS_STA_IDX,
      //  CPU::INS_STA_IDY,

      // CPU::INS_STX_ZP,
      // CPU::INS_STX_ZPY,
      // CPU::INS_STX_ABS,

      // CPU::INS_STY_ZP,
      // CPU::INS_STY_ZPX,
      // CPU::INS_STY_ABS,

      /* ===== MOVE DATA BETWEEN REGISTERS ===== */
      // CPU::INS_TAX,
      // CPU::INS_TAY,
      // CPU::INS_TXA,
      // CPU::INS_TYA,

      /* ===== INCREMENTING AND DECREMENTING ===== */
      // CPU::INS_DEX,
      // CPU::INS_DEY,
      // CPU::INS_INX,
      // CPU::INS_INX,

      // CPU::INS_DEC_ZP,
      // CPU::INS_DEC_ZPX,
      // CPU::INS_DEC_ABS,
      // CPU::INS_DEC_ABSX,

      // CPU::INS_INC_ZP,
      // CPU::INS_INC_ZPX,
      // CPU::INS_INC_ABS,
      // CPU::INS_INC_ABSX,

      /* ===== BRANCHING ===== */
      /* NOT TESTED YET */
      // CPU::INS_BNE,
      // CPU::INS_BEQ,
      // CPU::INS_BPL,
      // CPU::INS_BMI,
      // CPU::INS_BCC,
      // CPU::INS_BCS,
      // CPU::INS_BVC,
      // CPU::INS_BVS,

      /* ===== COMPARISON ===== */
      // CPU::INS_CMP_IM,
      // CPU::INS_CMP_ZP,
      // CPU::INS_CMP_ZPX,
      // CPU::INS_CMP_ABS,
      // CPU::INS_CMP_ABSX,
      // CPU::INS_CMP_ABSY,
      // CPU::INS_CMP_IDX,
      // CPU::INS_CMP_IDY,

      // CPU::INS_CPX_IM,
      // CPU::INS_CPX_ZP,
      // CPU::INS_CPX_ABS,

      // CPU::INS_CPY_IM,
      // CPU::INS_CPY_ZP,
      // CPU::INS_CPY_ABS,

      /* ===== JUMPS ===== */
      // CPU::INS_JMP_ABS,
      // CPU::INS_JMP_ID,

      // CPU::INS_JSR,
      // CPU::INS_RTS,

      /* TODO: FAILED */
      // CPU::INS_RTI,
      
      /* ===== BREAK ===== */
      /* NOT TESTED YET */
      // CPU::INS_BRK,

      /* ===== NOP ===== */
      // CPU::INS_NOP,

      /* ===== ARITHMETIC OPERATIONS ===== */

      /* ===== PSR OPERATIONS ===== */
      // CPU::INS_CLC,
      // CPU::INS_SEC,
      // CPU::INS_CLV,
      // CPU::INS_SED,
      // CPU::INS_CLD,

      /* TODO: AFTER OTHER TESTS */
      // CPU::INS_CLI,
      // CPU::INS_SEI,

      /* ===== LOGICAL OPERATIONS ===== */
      // CPU::INS_AND_IM,
      // CPU::INS_AND_ZP,
      // CPU::INS_AND_ZPX,
      // CPU::INS_AND_ABS,
      // CPU::INS_AND_ABSX,
      // CPU::INS_AND_ABSY,
      // CPU::INS_AND_IDX,
      // CPU::INS_AND_IDY,

      // CPU::INS_ORA_IM,
      // CPU::INS_ORA_ZP,
      // CPU::INS_ORA_ZPX,
      // CPU::INS_ORA_ABS,
      // CPU::INS_ORA_ABSX,
      // CPU::INS_ORA_ABSY,
      // CPU::INS_ORA_IDX,
      // CPU::INS_ORA_IDY,

      // CPU::INS_EOR_IM,
      // CPU::INS_EOR_ZP,
      // CPU::INS_EOR_ZPX,
      // CPU::INS_EOR_ABS,
      // CPU::INS_EOR_ABSX,
      // CPU::INS_EOR_ABSY,
      // CPU::INS_EOR_IDX,
      // CPU::INS_EOR_IDY,

      // /* ===== ROTATE AND SHIFT OPERATIONS ===== */
      // CPU::INS_ASL_A,
      // CPU::INS_ASL_ZP,
      // CPU::INS_ASL_ZPX,
      // CPU::INS_ASL_ABS,
      // CPU::INS_ASL_ABSX,
      
      // CPU::INS_LSR_A,
      // CPU::INS_LSR_ZP,
      // CPU::INS_LSR_ZPX,
      // CPU::INS_LSR_ABS,
      // CPU::INS_LSR_ABSX,
      
      // CPU::INS_ROL_A,
      // CPU::INS_ROL_ZP,
      // CPU::INS_ROL_ZPX,
      // CPU::INS_ROL_ABS,
      // CPU::INS_ROL_ABSX,
      
      // CPU::INS_ROR_A,
      // CPU::INS_ROR_ZP,
      // CPU::INS_ROR_ZPX,
      // CPU::INS_ROR_ABS,
      // CPU::INS_ROR_ABSX,

      // /* ===== STACK OPERATIONS ===== */
      // CPU::INS_TXS,
      // CPU::INS_TSX,
      // CPU::INS_PHA,
      // CPU::INS_PLA,
      // CPU::INS_PHP,
      // CPU::INS_PLP,

      // /* ===== Bit Test ===== */
      CPU::INS_BIT_ZP,
      // CPU::INS_BIT_ABS,
    }))
);