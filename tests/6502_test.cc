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
      format("tests/entries/{:x}.json", instruction)
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

      CPU::INS_INC_ZP,
      CPU::INS_INC_ZPX,
      CPU::INS_INC_ABS,
      CPU::INS_INC_ABSX,
    }))
);