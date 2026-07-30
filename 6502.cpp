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

    void Reset(u32 cycles, Memory& memory)
    {
        Word ResetVector = 0xFFFC;
        PC = ReadWord(cycles, ResetVector, memory);
        S = 0x0100;
        N = V = B = D = I = Z = C = 0;
        A = X = Y = 0;
    }

    void Execute(u32 cycles, Memory& memory)
    {
        while (cycles > 0) {
            Byte instruction = FetchByte(cycles, memory);
            printf("The current instruction is %04X\n", instruction);
            
            switch (instruction)
            {
                /* -------------------- LDA -------------------- */
                case INS_LDA_IM:    /* 2 cycles */
                {
                    Byte value = FetchByte(cycles, memory);
                    A = value;

                    LDSetStatus();
                } break;
                case INS_LDA_ZP:    /* 3 cycles */
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    Byte zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
                    A = zeroPageValue;

                    LDSetStatus();
                } break;
                case INS_LDA_ZPX:   /* 4 cycles */
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;

                    if (zeroPageAddress > ZERO_PAGE_END) {
                        return;
                    }

                    cycles--;

                    Byte zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    A = zeroPageValue;
                    
                    LDSetStatus();
                } break;
                case INS_LDA_ABS:   /* 4 cycles */
                {
                    Byte Address = FetchWord(cycles, memory);
                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;
                    LDSetStatus();
                } break;
                case INS_LDA_ABSX:  /* 4 cycles (+1 if page crossed) */
                {
                    Byte Address = FetchWord(cycles, memory);
                    Address += X;
                    cycles--;

                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;

                    LDSetStatus();
                } break;
                case INS_LDA_ABSY:  /* 4 cycles (+1 if page crossed) */
                {
                    Word Address = FetchWord(cycles, memory);
                    Address += Y;
                    cycles--;

                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;

                    LDSetStatus();
                } break;
                case INS_LDA_INDX:  /* 6 cycles */
                {
                    // LDA ($40,X)
                    Byte Address = FetchByte(cycles, memory);

                    Address += X;
                    cycles--;

                    Word AddressValue = ReadWord(cycles, Address, memory);
                    A = memory[AddressValue];

                    LDSetStatus();
                } break;
                case INS_LDA_INDY:  /* 5 cycles (+1 if page crossed) */
                {
                    // LDA ($40),Y
                    Byte Address = FetchByte(cycles, memory);
                    Word AddressValue = ReadWord(cycles, Address, memory);

                    AddressValue += Y;
                    cycles--;

                    Word FinalValue = ReadWord(cycles, AddressValue, memory);
                    A = FinalValue;

                    LDSetStatus();
                } break;




                /* -------------------- LDX -------------------- */
                case INS_LDX_IM:    /* 2 cycles */
                {
                    Byte value = FetchByte(cycles, memory);
                    X = value;

                    LDSetStatus();
                } break;
                case INS_LDX_ZP:    /* 3 cycles */
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    Byte zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
                    X = zeroPageValue;

                    LDSetStatus();
                } break;
                case INS_LDX_ZPY:   /* 4 cycles */
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += Y;

                    if (zeroPageAddress > ZERO_PAGE_END) {
                        return;
                    }

                    cycles--;

                    Byte zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    X = zeroPageValue;
                    
                    LDSetStatus();
                } break;
                case INS_LDX_ABS:   /* 4 cycles */
                {
                    Byte Address = FetchWord(cycles, memory);
                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    X = AddressValue;
                    LDSetStatus();
                } break;
                case INS_LDX_ABSY:  /* 4 cycles (+1 if page crossed) */
                {
                    Word Address = FetchWord(cycles, memory);
                    Address += Y;
                    cycles--;

                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    X = AddressValue;

                    LDSetStatus();
                } break;




                /* -------------------- LDY -------------------- */
                case INS_LDY_IM:    /* 2 cycles */
                {
                    Byte value = FetchByte(cycles, memory);
                    Y = value;

                    LDSetStatus();
                } break;
                case INS_LDY_ZP:    /* 3 cycles */
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    Byte zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
                    Y = zeroPageValue;

                    LDSetStatus();
                } break;
                case INS_LDY_ZPX:   /* 4 cycles */
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;

                    if (zeroPageAddress > ZERO_PAGE_END) {
                        return;
                    }

                    cycles--;

                    Byte zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    Y = zeroPageValue;
                    
                    LDSetStatus();
                } break;
                case INS_LDY_ABS:   /* 4 cycles */
                {
                    Byte Address = FetchWord(cycles, memory);
                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    Y = AddressValue;
                    LDSetStatus();
                } break;
                case INS_LDY_ABSX:  /* 4 cycles (+1 if page crossed) */
                {
                    Word Address = FetchWord(cycles, memory);
                    Address += X;
                    cycles--;

                    Byte AddressValue = ReadByte(cycles, Address, memory);

                    Y = AddressValue;

                    LDSetStatus();
                } break;





                /* -------------------- STA -------------------- */
                case INS_STA_ZP:    /* 3 cycles */
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    memory[zeroPageAddress] = A;
                    cycles--;
                } break;
                case INS_STA_ZPX:   /* 4 cycles */
                {
                    Byte zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;
                    cycles--; 

                    memory[zeroPageAddress] = A;
                    cycles--; 
                } break;
                case INS_STA_ABS:   /* 4 cycles */
                {
                    Word Address = FetchWord(cycles, memory);
                
                    memory[Address] = A;
                    cycles--;
                } break;
                case INS_STA_ABSX:  /* 5 cycles */
                {
                    Word Address = FetchWord(cycles, memory);

                    Address += X;
                    cycles--;

                    memory[Address] = A;
                    cycles--;
                } break;
                case INS_STA_ABSY:  /* 5 cycles */
                {
                    Word Address = FetchWord(cycles, memory);

                    Address += Y;
                    cycles--;

                    memory[Address] = A;
                    cycles--;
                } break;
                case INS_STA_INDX:
                {
                    // STA ($40,X)
                    Byte Address = FetchWord(cycles, memory);

                    Address += X;
                    cycles--;

                    Word AddressValue = ReadWord(cycles, Address, memory);

                    memory[AddressValue] = A;
                } break;
                case INS_STA_INDY:
                {
                    // STA ($40),Y
                    Byte Address = FetchWord(cycles, memory);
                    Word AddressValue = ReadWord(cycles, Address, memory);

                    AddressValue += Y;
                    cycles--;

                    Word FinalValue = ReadWord(cycles, AddressValue, memory);
                    A = FinalValue;

                    memory[AddressValue] = A;
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
        /* LDA → Loads a byte of memory into the accumulator, setting zero and negative flags as appropriate */
        INS_LDA_IM = 0xA9,
        INS_LDA_ZP = 0xA5,
        INS_LDA_ZPX = 0xB5,
        INS_LDA_ABS = 0xAD,
        INS_LDA_ABSX = 0xBD,
        INS_LDA_ABSY = 0xB9,
        INS_LDA_INDX = 0xA1,
        INS_LDA_INDY = 0xB1,

        /* LDX → Loads a byte of memory into the X register, setting the zero and negative flags as appropriate. */
        INS_LDX_IM = 0xA2,
        INS_LDX_ZP = 0xA6,
        INS_LDX_ZPY = 0xB6,
        INS_LDX_ABS = 0xAE,
        INS_LDX_ABSY = 0xBE,

        /* LDY → Loads a byte of memory into the Y register, setting the zero and negative flags as appropriate. */
        INS_LDY_IM = 0xA0,
        INS_LDY_ZP = 0xA4,
        INS_LDY_ZPX = 0xB4,
        INS_LDY_ABS = 0xAC,
        INS_LDY_ABSX = 0xBC,

        /* STA → Stores the contents of the accumulator into memory. */
        INS_STA_ZP = 0x85,
        INS_STA_ZPX = 0x95,
        INS_STA_ABS = 0x8D,
        INS_STA_ABSX = 0x9D,
        INS_STA_ABSY = 0x99,
        INS_STA_INDX = 0x81,
        INS_STA_INDY = 0x91;

    void LDSetStatus()
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
    /* Initializing memory */
    Memory memory;
    memory.Initialize();

    /* Instructions */
    memory[0x8000] = CPU::INS_LDA_IM;
    memory[0x8001] = 0x30;
    memory[0x8002] = CPU::INS_STA_ABSX;
    memory[0x8003] = 0x57;
    memory[0x8004] = 0x15;

    /* Point the Reset Vector (0xFFFC/0xFFFD) to the program's start address */
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x80;

    /* Other memory locations */
    memory[0x30] = 0x15;

    /* Initializing CPU */
    CPU cpu;
    cpu.Reset(2, memory);
    cpu.X = 0x05;

    /* Execution */
    cpu.Execute(2, memory);

    /* Debugging */
    memory.Debug(0xFFF0, 0xFFFF);
    memory.Debug(0x1555, 0x1565);
    cpu.Debug();
    memory.DebugZeroPage();

    return 0;
}
