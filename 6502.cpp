#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <bitset>

using namespace std;

using Byte = unsigned char;     // 1 byte
using Word = unsigned short;    // 2 bytes
using u32 = unsigned int;       // 4 bytes

#define NORMAL  "\x1B[0m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE  "\x1B[34m"
#define MAGENTA  "\x1B[35m"
#define CYAN  "\x1B[36m"
#define WHITE  "\x1B[37m"

Byte STACK_ADDRESS = 0x0100;
Byte STACK_ADDRESS_END = 0x01FF;

Byte ZERO_PAGE = 0x0100;
Byte ZERO_PAGE_END = 0x0100;

Byte RESET_VECTOR = 0xFFFC;

struct Memory
{
    static constexpr u32 MAX_MEMORY = 1024 * 64;
    static constexpr u32 ZERO_PAGE_SIZE = 256;
    Byte Data[MAX_MEMORY];

    void Initialize()
    {
        for (u32 i = 0; i < MAX_MEMORY; i++)
        {
            Data[i] = 0;
        }
    }

    void Debug(u32 start, u32 end)
    {
        int counter = 0;
        for (u32 i = start; i < end; i++)
        {
            printf("[0x%04X:0x%04X]  ",i, Data[i]);
            
            if ((counter + 1) % 8 == 0) cout << endl;

            counter++;
        }

        cout << endl;
    }

    void DebugZeroPage()
    {
        printf("\n ---- ZERO PAGE MEMORY ---- \n");
        for (u32 i = 0; i < ZERO_PAGE_SIZE; i++)
        {
            u32 val = Data[i];
            
            if (val == 0)
                printf("%02X ", Data[i]);
            else
                printf("\e%s%02X\e[0m ", MAGENTA, Data[i]);

            if ((i + 1) % 8 == 0) cout << endl;
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

    /* write 2 bytes */
    Word writeWord(Word Value, u32 Address, u32 cycles) {
        Data[Address] = Value & 0xFF;
        Data[Address + 1] = (Value >> 8);

        cycles -= 2;
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
        printf("Executing... Cycle number %d\n", cycles);
        while (cycles > 0) {
            Byte instruction = FetchByte(cycles, memory);
            switch (instruction)
            {
                case INS_LDA_IM:
                {
                    Byte value = FetchByte(cycles, memory);
                    A = value;

                    LDASetStatus();
                } break;
                case INS_LDA_ZP:
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    Byte zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
                    A = zeroPageValue;

                    LDASetStatus();
                } break;
                case INS_LDA_ZPX:
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;

                    if (zeroPageAddress > ZERO_PAGE_END) {
                        printf("Address overflow, size is greater than the Zero Page");
                        
                        return;
                    }

                    cycles--;

                    Byte zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    A = zeroPageValue;
                    
                    LDASetStatus();
                } break;
                case INS_LDA_ABS:
                {
                    Byte Address = FetchWord(cycles, memory);
                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;
                    LDASetStatus();
                } break;
                case INS_LDA_ABSX:
                {
                    Byte Address = FetchWord(cycles, memory);
                    Address += X;
                    cycles--;

                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;

                    LDASetStatus();
                } break;
                case INS_LDA_ABSY:
                {
                    Word Address = FetchWord(cycles, memory);
                    Address += Y;
                    cycles--;

                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;

                    LDASetStatus();
                } break;
                case INS_LDA_INDX:
                {
                    // LDA ($40,X)
                    Word Address = FetchWord(cycles, memory);

                    Address += X;
                    cycles--;

                    Word AddressValue = ReadWord(cycles, Address, memory);

                    A = memory[AddressValue];

                    LDASetStatus();
                } break;
                case INS_LDA_INDY:
                {
                    // LDA ($40),Y
                    Word Address = FetchWord(cycles, memory);
                    Word AddressValue = ReadWord(cycles, Address, memory);

                    AddressValue += Y;
                    cycles--;

                    Word FinalValue = ReadWord(cycles, AddressValue, memory);
                    A = FinalValue;

                    LDASetStatus();
                } break;

                /* JUMPS */
                case INS_JSR:
                {
                    Word SubAddress = FetchWord(cycles, memory);
                    memory[S] = PC - 1;
                    S--;
                    cycles--;

                    PC = SubAddress;
                    cycles--;
                } break;
                default:
                {
                    /* TODO: Exception object */
                    printf("Instruction not handled %d", instruction);

                    cycles--;
                } break;
            }
        }
    }

    Byte FetchByte(u32& cycles, Memory &memory) {
        Byte Data = memory[PC];
        printf("The byte fetched is %04X\n", Data);
        PC++;
        cycles--;

        return Data;
    }

    /* Same as the previous function but doesn't increment the program counter */
    Byte ReadByte(u32& cycles, u32 address, Memory &memory) {
        Byte Data = memory[address];
        printf("The byte read is %04X\n", Data);
        cycles--;

        return Data;
    }

    /* TODO: adding a swap byte function depending on whether the platform is little or big endian. */
    Word FetchWord(u32& cycles, Memory &memory) {
        /* 6502 is little endian */
        Word Data = memory[PC];         /* Lower byte */
        PC++;

        Data |= (memory[PC] << 8);     /* Higher byte */
        PC++;

        cycles -= 2;

        return Data;
    }

    Word ReadWord(u32& cycles, u32 address, Memory &memory) {
        /* 6502 is little endian */
        Word Data = memory[address];            /* Lower byte */
        printf("Address being read is: %04X, with value: %04X\n", address, Data);


        Data |= (memory[address + 1] << 8);     /* Higher byte */
        printf("Address being read is: %04X, with value: %04X\n", address + 1, Data);

        cycles -= 2;

        return Data;
    }

    // opcodes
    static constexpr Byte
        /* LDA */
        INS_LDA_IM = 0xA9,
        INS_LDA_ZP = 0xA5,
        /* UNFINISHED */
        INS_LDA_ZPX = 0xB5,
        INS_LDA_ABS = 0xAD,
        INS_LDA_ABSX = 0xBD,
        INS_LDA_ABSY = 0xB9,
        INS_LDA_INDX = 0xA1,
        INS_LDA_INDY = 0xB1,

        /* JUMPS */
        INS_JSR = 0x20;

    void LDASetStatus()
    {
        Z = (A == 0);
        N = (A & 0b10000000) > 0;
    }

    void Debug() {
        printf("A: %04X\nX: %04X\nY: %04X", A, X, Y);
    }
};


int main()
{
    Memory memory;
    CPU cpu;
    cpu.Reset(memory);
    cpu.Y = 0x05;

    memory[0xFFFC] = CPU::INS_LDA_INDX;
    memory[0xFFFD] = 0x30;
    memory[0xFFFE] = 0x00;
    memory[0x0030] = 0x60;
    memory[0x0031] = 0x70;
    memory[0x7060] = 0xF3;

    printf("In address 0x6880: %04X\n", memory[0x6880]);

    cpu.Execute(6, memory);

    memory.Debug(0xFFF0, 0xFFFF);
    cpu.Debug();
    memory.DebugZeroPage();

    return 0;
}
