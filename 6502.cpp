#include "6502.h"

int main()
{
    /* Initializing memory */
    Memory memory;
    memory.Initialize();

    /* Instructions */
    vector<uint32_t> instructions = {
        CPU::INS_SED,             // 2
        CPU::INS_LDA_IM, 0x5e,    // 2 
        CPU::INS_ADC_IM, 0xe6,    // 3
        CPU::INS_BRK,
    };

    /* Point the Reset Vector (0xFFFC/0xFFFD) to the program's start address */
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x80;

    /* Filling up some memory addresses */
    memory[0x0084] = 0x15;
    memory[0x0094] = 0x20;
    memory[0x8415] = 0x25;
    memory[0x8425] = 0x30;  // prev + X
    memory[0x841A] = 0x35;  // prev + Y

    /* Initializing CPU */
    CPU cpu;
    cpu.Reset(2, memory);
    cpu.MountProgram(instructions, memory, instructions.size());
    
    /* Execution */
    uint32_t cycles = cpu.Execute(68, memory);

    /* Debugging */
    // memory.Debug(0x1055, 0x1065);
    cpu.Debug();
    // memory.DebugPage(0);
    // memory.DebugPage(0x0001);
    // memory.DebugPage(0x0002);
    printf("Cycles consumed: %d.\n", cycles);

    return 0;
}