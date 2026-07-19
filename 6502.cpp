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

    /* read 1 byte */
    Byte operator[](u32 address) const {
        return Data[address];
    }

    /* write 1 byte */
    Byte& operator[](u32 address) {
        return Data[address];
    }
};

struct CPU
{
    // Program Counter
    Word PC;

    // Registers: Accumulator, Stack Pointer, Index Registers
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

    void Execute(u32 cycles, Memory& memory)
    {
        while (cycles > 0) {
            Byte instruction = FetchByte(cycles, memory);
            switch (instruction)
            {
                case INS_LDA_IM:
                {
                    Byte value = FetchByte(cycles, memory);
                    A = value;

                    Z = (A == 0);
                    N = (A & 0b10000000) > 0;
                } break;
                default:
                {
                    printf("Instruction not handled %d", instruction);
                } break;
            }
        }
    }

    Byte FetchByte(u32& cycles, Memory &memory) {
        Byte data = memory[PC];
        PC++;
        cycles--;

        return data;
    }

    // opcodes
    static constexpr Byte
        INS_LDA_IM = 0xA9;
};


int main()
{
    Memory memory;
    CPU cpu;
    cpu.Reset(memory);

    memory[0xFFFC] = CPU::INS_LDA_IM;
    memory[0xFFFD] = 0x42;

    cpu.Execute(2, memory);

    cout << memory[0xFFFD] << endl;

    return 0;
}
