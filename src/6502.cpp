#include "6502.h"

int main()
{
    /* Initializing memory */
    Memory memory;
    memory.Initialize();

    /* Instructions */
    memory[12968] = 208;
    memory[12969] = 221;
    memory[12970] = 121;

    /* Initializing CPU */
    CPU cpu;
    cpu.Reset(3, memory);

    /* Setting up registers */
    cpu.PC = 12968;
    cpu.S = 35;
    cpu.A = 251;
    cpu.X = 13;
    cpu.Y = 151;
    cpu.P = 174;

    /* Execution */
    uint32_t cycles = cpu.Execute(2, memory);

    /* Debugging */
    cpu.Debug();
    // memory.DebugPage(1);
    // memory.DebugPage(0x0001);
    printf("Cycles consumed: %d.\n", cycles);

    printf("PC: %d\n S: %d\n A: %d\n X: %d\n Y: %d\n P: %d\n ", cpu.PC, cpu.S, cpu.A, cpu.X, cpu.Y, cpu.P);


    return 0;
}