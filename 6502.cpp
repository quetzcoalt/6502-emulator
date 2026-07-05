#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

using Byte = unsigned char;
using Word = unsigned short;
using u32 = unsigned int;

Byte STACK_ADDRESS = 0x0100;
Byte STACK_ADDRESS_END = 0x01FF;

Byte ZERO_PAGE = 0x0100;
Byte ZERO_PAGE_END = 0x0100;

Byte RESET_VECTOR = 0xFFFC;

struct Memory
{
    static constexpr u32 MAX_MEMORY = 1014 * 64;
    Byte Data[MAX_MEMORY];

    void Initialize()
    {
        for (u32 i = 0; i < MAX_MEMORY; i++)
        {
            Data[i] = 0;
        }
    }
};

struct CPU
{
    // Program Counter
    Word PC;

    // Registers: Accumulator, Stack POinter, Index Registers
    Byte A, S, X, Y;

    // Status flag
    Byte N : 1;
    Byte V : 1;
    Byte B : 1;
    Byte D : 1;
    Byte I : 1;
    Byte Z : 1;
    Byte C : 1;

    void Reset(Memory& memory)
    {
        PC = 0xFFFC;
        S = 0x0100;
        N = V = B = D = I = Z = C = 0;
        A = X = Y = 0;

        memory.Initialize();
    }
};


int main()
{
    Memory memory;
    CPU cpu;
    cpu.Reset(memory);

    return 0;
}
