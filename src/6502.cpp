#include "6502.h"

/* For JSON tests */
// #include <nlohmann/json.hpp>
// #include <fstream>

// nlohmann::json LoadJson()
// {
//     std::ifstream file("tests/entries/bd.json");

//     nlohmann::json json;
//     file >> json;

//     return json;
// }

// int main()
// {
//     int test_number = 0;
//     nlohmann::json tests = LoadJson();

//     cout << tests[test_number] << endl;

//     /* Initializing memory */
//     Memory memory;
//     memory.Initialize();

//     /* Instructions */
//     vector<uint32_t> instructions = {
//         CPU::INS_LDA_IM, 0x9b,      // 2
//         CPU::INS_STA_ABS, 0x00, 0x01,   // 4
//         CPU::INS_PLP,               // 4
//     };

//     /* Point the Reset Vector (0xFFFC/0xFFFD) to the program's start address */
//     memory[0xFFFC] = 0x00;
//     memory[0xFFFD] = 0x80;

//     /* Initializing CPU */
//     CPU cpu;
//     cpu.Reset(3, memory);
//     // cpu.MountProgram(instructions, memory, instructions.size());

//     /* Setting up registers */
//     cpu.PC = tests[test_number]["initial"]["pc"];
//     cpu.S = tests[test_number]["initial"]["s"];
//     cpu.A = tests[test_number]["initial"]["a"];
//     cpu.X = tests[test_number]["initial"]["x"];
//     cpu.Y = tests[test_number]["initial"]["y"];
//     cpu.P = tests[test_number]["initial"]["p"];

//     printf("PC: %d\n S: %d\n A: %d\n X: %d\n Y: %d\n P: %d\n ", cpu.PC, cpu.S, cpu.A, cpu.X, cpu.Y, cpu.P);
    
//     // cout << tests["initial"]["ram"] << endl;
//     for (auto mem : tests[test_number]["initial"]["ram"])
//     {
//         memory[mem[0]] = mem[1];
//     }

//     /* Execution */
//     uint32_t cycles = cpu.Execute(tests[test_number]["cycles"].size(), memory);

//     /* Debugging */
//     cpu.Debug();
//     // memory.DebugPage(1);
//     // memory.DebugPage(0x0001);
//     printf("Cycles consumed: %d.\n", cycles);

//     return 0;
// }

/* For debugging */
int main()
{
    /* Initializing memory */
    Memory memory;
    memory.Initialize();

    /* Instructions */
    memory[42677] = 161;
    memory[42678] = 176;
    memory[42679] = 161;
    memory[176] = 138;
    memory[62] = 24;
    memory[63] = 91;
    memory[23320] = 69;

    /* Initializing CPU */
    CPU cpu;
    cpu.Reset(3, memory);

    /* Setting up registers */
    cpu.PC = 42677;
    cpu.S = 178;
    cpu.A = 4;
    cpu.X = 142;
    cpu.Y = 1;
    cpu.P = 38;

    /* Execution */
    uint32_t cycles = cpu.Execute(6, memory);

    /* Debugging */
    cpu.Debug();
    // memory.DebugPage(1);
    // memory.DebugPage(0x0001);
    printf("Cycles consumed: %d.\n", cycles);

    return 0;
}