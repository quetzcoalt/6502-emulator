#include "6502.h"

int main()
{
    /* Initializing memory */
    Memory memory;
    memory.Initialize();

    /* Instructions */
    vector<uint32_t> instructions = {
        CPU::INS_JSR, 0x18, 0x03,   // 6
        CPU::INS_TAX,               // 2
        CPU::INS_TYA,               // 2
        CPU::INS_INY,               // 2
        CPU::INS_BRK,
    };

    /* Point the Reset Vector (0xFFFC/0xFFFD) to the program's start address */
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x80;

    /* Other memory locations */
    memory[0x0318] = CPU::INS_LDA_IM;       // 2
    memory[0x0319] = 0x17;
    memory[0x031A] = CPU::INS_LDX_IM;       // 6
    memory[0x031B] = 0x18;
    memory[0x031C] = CPU::INS_LDY_IM;       // 6
    memory[0x031D] = 0x19;
    memory[0x031E] = CPU::INS_RTS;          // 6

    /* Initializing CPU */
    CPU cpu;
    cpu.Reset(2, memory);
    cpu.MountProgram(instructions, memory, instructions.size());
    cpu.X = 0x05;
    cpu.P = 0;

    /* Execution */
    cpu.Execute(32, memory);

    /* Debugging */
    // memory.Debug(0x1055, 0x1065);
    cpu.Debug();
    memory.DebugPage(0);
    memory.DebugPage(0x0001);
    memory.DebugPage(0x0002);
    memory.DebugPage(0x0003);

    return 0;
}