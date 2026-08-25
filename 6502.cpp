#include "6502.h"

int main()
{
    /* Initializing memory */
    Memory memory;
    memory.Initialize();

    /* Instructions */
    vector<uint32_t> instructions = {
        CPU::INS_LDA_IM, 0x9b,      // 2
        CPU::INS_STA_ABS, 0x00, 0x01,   // 4
        CPU::INS_PLP,               // 4
    };

    /* Point the Reset Vector (0xFFFC/0xFFFD) to the program's start address */
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x80;

    /* Initializing CPU */
    CPU cpu;
    cpu.Reset(3, memory);
    cpu.MountProgram(instructions, memory, instructions.size());
    
    /* Execution */
    uint32_t cycles = cpu.Execute(11, memory);

    /* Debugging */
    // memory.Debug(0x1055, 0x1065);
    cpu.Debug();
    memory.DebugPage(1);
    // memory.DebugPage(0x0001);
    printf("Cycles consumed: %d.\n", cycles);

    return 0;
}