#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <bitset>

using namespace std;

// using uint8_t = unsigned char;     // 1 uint8_t
// using uint16_t = unsigned short;    // 2 bytes
// using int32_t = unsigned int;       // 4 bytes

#define NORMAL  "\x1B[0m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE  "\x1B[34m"
#define MAGENTA  "\x1B[35m"
#define CYAN  "\x1B[36m"
#define WHITE  "\x1B[37m"

uint8_t STACK_ADDRESS = 0x0100;
uint8_t STACK_ADDRESS_END = 0x01FF;

uint8_t ZERO_PAGE = 0x0100;
uint8_t ZERO_PAGE_END = 0x0100;

uint8_t RESET_VECTOR = 0xFFFC;

struct Memory
{
    static constexpr int32_t MAX_MEMORY = 1024 * 64;
    static constexpr uint8_t PAGE_SIZE = 0xFF;
    uint8_t Data[MAX_MEMORY];

    void Initialize()
    {
        for (int32_t i = 0; i < MAX_MEMORY; i++)
        {
            Data[i] = 0;
        }
    }

    void Debug(int32_t start, int32_t end)
    {
        int counter = 0;
        for (int32_t i = start; i < end; i++)
        {
            printf("[0x%04X:0x%04X]  ",i, Data[i]);
            
            if ((counter + 1) % 8 == 0) cout << endl;

            counter++;
        }

        cout << endl;
    }

    void DebugPage(uint8_t page)
    {
        if (page == 0)
        {
            printf("\n ---- ZERO PAGE MEMORY ---- \n");
        } else if (page == 1)
        {
            printf("\n ---- STACK MEMORY ---- \n");
        } else 
        {
            printf("\n ---- PAGE %04x ---- \n", page);
        }

        uint32_t PageBoundary = page << 8;
        PageBoundary |= PAGE_SIZE;

        uint32_t PageStart = page << 8;

        for (int32_t i = PageStart; i <= PageBoundary; i++)
        {
            int32_t val = Data[i];
            
            if (val == 0)
                printf("%02X ", Data[i]);
            else
                printf("\e%s%02X\e[0m ", MAGENTA, Data[i]);

            if ((i + 1) % 8 == 0) cout << endl;
        }
    }

    /* read 1 uint8_t */
    uint8_t operator[](int32_t address) const {
        return Data[address];
    }

    /* write 1 uint8_t */
    uint8_t& operator[](int32_t address) {
        return Data[address];
    }

    /* write 2 bytes */
    void WriteWord(uint16_t Value, int32_t address, uint32_t cycles) {
        Data[address] = Value >> 8;
        Data[address - 1] = Value & 0xFF;

        cycles -= 2;
    }
};

struct CPU
{
    // Program Counter
    uint16_t PC;

    // Registers: Accumulator, Index Registers
    uint8_t A, X, Y;

    // Stack Pointer
    uint16_t S;

    // Status register
    /*
     * [0] C → carry
     * [1] Z → zero
     * [2] I → interrupt disable
     * [3] D → decimal
     * [4] B → break
     * [5] unused, forced to 1 when something is pushed to the stack
     * [6] V → overflow
     * [7] N → negative
    */
    uint8_t P;

    void Reset(int32_t cycles, Memory& memory)
    {
        uint16_t ResetVector = 0xFFFC;
        PC = ReadWord(cycles, ResetVector, memory);
        S = 0x01FF;
        P = 0;
        A = X = Y = 0;
    }
    
    /* Insert program into memory */
    void MountProgram(vector<uint32_t> instructions, Memory& memory, int32_t cycles) {
        int32_t StartAddress = PC;
        
        for (uint8_t ins : instructions) {
            if (cycles > 0) {
                memory[StartAddress] = ins;
                StartAddress++;
                cycles--;
            }
        }
    }

    void Execute(int32_t cycles, Memory& memory)
    {
        while (cycles > 0) {
            uint8_t instruction = FetchByte(cycles, memory);
            printf("Instruction being read is: %04x\n", instruction);
            
            switch (instruction)
            {
                /* -------------------- LDA -------------------- */
                case INS_LDA_IM:    /* 2 cycles */
                {
                    uint8_t value = FetchByte(cycles, memory);
                    A = value;

                    LDSetStatus();
                } break;
                case INS_LDA_ZP:    /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    int8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
                    A = zeroPageValue;

                    LDSetStatus();
                } break;
                case INS_LDA_ZPX:   /* 4 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;

                    if (zeroPageAddress > ZERO_PAGE_END) {
                        return;
                    }

                    cycles--;

                    uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    A = zeroPageValue;
                    
                    LDSetStatus();
                } break;
                case INS_LDA_ABS:   /* 4 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;
                    LDSetStatus();
                } break;
                case INS_LDA_ABSX:  /* 4 cycles (+1 if page crossed) */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint16_t TargetAddress = Address + X;

                    if ((Address && 0xFF00) != (TargetAddress && 0xFF00))
                    {
                        cycles--;
                    }

                    uint8_t AddressValue = ReadByte(cycles, TargetAddress, memory);

                    A = TargetAddress;

                    LDSetStatus();
                } break;
                case INS_LDA_ABSY:  /* 4 cycles (+1 if page crossed) */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint16_t TargetAddress = Address + Y;

                    if ((Address && 0xFF00) != (TargetAddress && 0xFF00))
                    {
                        cycles--;
                    }

                    uint8_t AddressValue = ReadByte(cycles, TargetAddress, memory);

                    A = TargetAddress;

                    LDSetStatus();
                } break;
                case INS_LDA_IDX:  /* 6 cycles */
                {
                    // LDA ($40,X)
                    uint8_t Address = FetchByte(cycles, memory);

                    Address += X;
                    cycles--;

                    uint16_t AddressValue = ReadWord(cycles, Address, memory);
                    A = ReadByte(cycles, Address, memory);

                    LDSetStatus();
                } break;
                case INS_LDA_IDY:  /* 5 cycles (+1 if page crossed) */
                {
                    // LDA ($40),Y
                    uint8_t Address = FetchByte(cycles, memory);
                    uint16_t AddressValue = ReadWord(cycles, Address, memory);

                    uint16_t TargetAddress = AddressValue + Y;

                    if ((Address && 0xFF00) != (TargetAddress && 0xFF00))
                    {
                        cycles--;
                    }

                    A = ReadByte(cycles, TargetAddress, memory);;

                    LDSetStatus();
                } break;




                /* -------------------- LDX -------------------- */
                case INS_LDX_IM:    /* 2 cycles */
                {
                    uint8_t value = FetchByte(cycles, memory);
                    X = value;

                    LDSetStatus();
                } break;
                case INS_LDX_ZP:    /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
                    X = zeroPageValue;

                    LDSetStatus();
                } break;
                case INS_LDX_ZPY:   /* 4 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += Y;

                    if (zeroPageAddress > ZERO_PAGE_END) {
                        return;
                    }

                    cycles--;

                    uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    X = zeroPageValue;
                    
                    LDSetStatus();
                } break;
                case INS_LDX_ABS:   /* 4 cycles */
                {
                    uint8_t Address = FetchWord(cycles, memory);
                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    X = AddressValue;
                    LDSetStatus();
                } break;
                case INS_LDX_ABSY:  /* 4 cycles (+1 if page crossed) */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint16_t TargetAddress = Address + Y;

                    if ((Address && 0xFF00) != (TargetAddress && 0xFF00))
                    {
                        cycles--;
                    }

                    uint8_t AddressValue = ReadByte(cycles, TargetAddress, memory);

                    X = AddressValue;

                    LDSetStatus();
                } break;




                /* -------------------- LDY -------------------- */
                case INS_LDY_IM:    /* 2 cycles */
                {
                    uint8_t value = FetchByte(cycles, memory);
                    Y = value;

                    LDSetStatus();
                } break;
                case INS_LDY_ZP:    /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
                    Y = zeroPageValue;

                    LDSetStatus();
                } break;
                case INS_LDY_ZPX:   /* 4 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;

                    if (zeroPageAddress > ZERO_PAGE_END) {
                        return;
                    }

                    cycles--;

                    uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    Y = zeroPageValue;
                    
                    LDSetStatus();
                } break;
                case INS_LDY_ABS:   /* 4 cycles */
                {
                    uint8_t Address = FetchWord(cycles, memory);
                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    Y = AddressValue;
                    LDSetStatus();
                } break;
                case INS_LDY_ABSX:  /* 4 cycles (+1 if page crossed) */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint16_t TargetAddress = Address + X;

                    if ((Address && 0xFF00) != (TargetAddress && 0xFF00))
                    {
                        cycles--;
                    }

                    uint8_t AddressValue = ReadByte(cycles, TargetAddress, memory);

                    Y = AddressValue;

                    LDSetStatus();
                } break;





                /* -------------------- STA -------------------- */
                case INS_STA_ZP:    /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    memory[zeroPageAddress] = A;
                    cycles--;
                } break;
                case INS_STA_ZPX:   /* 4 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;
                    cycles--; 

                    memory[zeroPageAddress] = A;
                    cycles--; 
                } break;
                case INS_STA_ABS:   /* 4 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                
                    memory[Address] = A;
                    cycles--;
                } break;
                case INS_STA_ABSX:  /* 5 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);

                    Address += X;
                    cycles--;

                    memory[Address] = A;
                    cycles--;
                } break;
                case INS_STA_ABSY:  /* 5 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);

                    Address += Y;
                    cycles--;

                    memory[Address] = A;
                    cycles--;
                } break;
                case INS_STA_IDX:
                {
                    // STA ($40,X)
                    uint8_t Address = FetchWord(cycles, memory);

                    Address += X;
                    cycles--;

                    uint16_t AddressValue = ReadWord(cycles, Address, memory);

                    memory[AddressValue] = A;
                } break;
                case INS_STA_IDY:
                {
                    // STA ($40),Y
                    uint8_t Address = FetchWord(cycles, memory);
                    uint16_t AddressValue = ReadWord(cycles, Address, memory);

                    AddressValue += Y;
                    cycles--;

                    uint16_t FinalValue = ReadWord(cycles, AddressValue, memory);
                    A = FinalValue;

                    memory[AddressValue] = A;
                } break;



                /* ---------- STX ---------- */
                case INS_STX_ZP:    /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    memory[zeroPageAddress] = X;
                    cycles--;
                } break;
                case INS_STX_ZPY:   /* 4 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += Y;
                    cycles--; 

                    memory[zeroPageAddress] = X;
                    cycles--; 
                } break;
                case INS_STX_ABS:   /* 4 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                
                    memory[Address] = X;
                    cycles--;
                } break;




                /* ---------- STY ---------- */
                case INS_STY_ZP:    /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    memory[zeroPageAddress] = Y;
                    cycles--;
                } break;
                case INS_STY_ZPX:   /* 4 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;
                    cycles--; 

                    memory[zeroPageAddress] = Y;
                    cycles--; 
                } break;
                case INS_STY_ABS:   /* 4 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                
                    memory[Address] = Y;
                    cycles--;
                } break;




                /* ---------- TRANSFER BETWEEN REGISTERS ---------- */
                case INS_TAX:       /* 2 cycles */
                {
                    X = A;
                    cycles--;
                    LDSetStatus();
                } break;
                case INS_TAY:       /* 2 cycles */
                {
                    Y = A;
                    cycles--;
                    LDSetStatus();
                } break;
                case INS_TXA:       /* 2 cycles */
                {
                    A = X;
                    cycles--;
                    LDSetStatus();
                } break;
                case INS_TYA:       /* 2 cycles */
                {
                    A = Y;
                    cycles--;
                    LDSetStatus();
                } break;




                /* ---------- INCREMENTING AND DECREMENTING REGISTERS ---------- */
                case INS_DEX:       /* 2 cycles */
                {
                    X--;
                    cycles--;
                    LDSetStatus();
                } break;
                case INS_DEY:       /* 2 cycles */
                {
                    Y--;
                    cycles--;
                    LDSetStatus();
                } break;
                case INS_INX:       /* 2 cycles */
                {
                    X++;
                    cycles--;
                    LDSetStatus();
                } break;
                case INS_INY:       /* 2 cycles */
                {
                    Y++;
                    cycles--;
                    LDSetStatus();
                } break;




                /* ---------- INCREMENTING AND DECREMENTING A MEMORY VALUE ---------- */
                case INS_DEC_ZP:        /* 5 cycles */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    
                    uint8_t Value = memory[Address];
                    cycles--;
                    
                    Value--;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;
                } break;
                case INS_DEC_ZPX:       /* 6 cycles */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    Address += X;
                    cycles--;
                    
                    uint8_t Value = memory[Address];
                    cycles--;
                    
                    Value--;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;
                } break;
                case INS_DEC_ABS:       /* 6 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    
                    uint8_t Value = memory[Address];
                    cycles--;
                    
                    Value--;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;
                } break;
                case INS_DEC_ABSX:      /* 7 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    Address += X;
                    cycles--;
                    
                    uint8_t Value = memory[Address];
                    cycles--;
                    
                    Value--;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;
                } break;

                case INS_INC_ZP:        /* 5 cycles */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    
                    uint8_t Value = memory[Address];
                    cycles--;
                    
                    Value++;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;
                } break;
                case INS_INC_ZPX:       /* 6 cycles */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    Address += X;
                    cycles--;
                    
                    uint8_t Value = memory[Address];
                    cycles--;
                    
                    Value++;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;
                } break;
                case INS_INC_ABS:       /* 6 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    
                    uint8_t Value = memory[Address];
                    cycles--;
                    
                    Value++;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;
                } break;
                case INS_INC_ABSX:      /* 7 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    Address += X;
                    cycles--;
                    
                    uint8_t Value = memory[Address];
                    cycles--;
                    
                    Value++;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;
                } break;




                /* ---------- BRANCHING ---------- */
                case INS_BNE:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    uint16_t oldPC = PC;
                    if ((P & 0b00000010) == 0) {
                        uint8_t address = FetchByte(cycles, memory);
                        PC += 1 + address;
                        cycles--;

                        if ((oldPC >> 8) != (PC >> 8)) cycles--;
                    }
                } break;
                case INS_BEQ:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    uint16_t oldPC = PC;
                    if ((P & 0b00000010) == 0b00000010) {
                        uint8_t address = FetchByte(cycles, memory);
                        PC += 1 + address;
                        cycles--;

                        if ((oldPC >> 8) != (PC >> 8)) cycles--;
                    }
                } break;
                case INS_BPL:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    uint16_t oldPC = PC;
                    if ((P & 0b10000000) == 0) {
                        uint8_t address = FetchByte(cycles, memory);
                        PC += 1 + address;
                        cycles--;

                        if ((oldPC >> 8) != (PC >> 8)) cycles--;
                    }
                } break;
                case INS_BMI:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    uint16_t oldPC = PC;
                    if ((P & 0b10000000) == 0b10000000) {
                        uint8_t address = FetchByte(cycles, memory);
                        PC += 1 + address;
                        cycles--;

                        if ((oldPC >> 8) != (PC >> 8)) cycles--;
                    }
                } break;
                case INS_BCC:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    uint16_t oldPC = PC;
                    if ((P & 0b00000001) == 0) {
                        uint8_t address = FetchByte(cycles, memory);
                        PC += 1 + address;
                        cycles--;

                        if ((oldPC >> 8) != (PC >> 8)) cycles--;
                    }
                } break;
                case INS_BCS:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    uint16_t oldPC = PC;
                    if ((P & 0b00000001) == 0b00000001) {
                        uint8_t address = FetchByte(cycles, memory);
                        PC += 1 + address;
                        cycles--;

                        if ((oldPC >> 8) != (PC >> 8)) cycles--;
                    }
                } break;
                case INS_BVC:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    uint16_t oldPC = PC;
                    if ((P & 0b01000000) == 0) {
                        uint8_t address = FetchByte(cycles, memory);
                        PC += 1 + address;
                        cycles--;

                        if ((oldPC >> 8) != (PC >> 8)) cycles--;
                    }
                } break;
                case INS_BVS:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    uint16_t oldPC = PC;
                    if ((P & 0b01000000) == 0b01000000) {
                        uint8_t address = FetchByte(cycles, memory);
                        PC += 1 + address;
                        cycles--;

                        if ((oldPC >> 8) != (PC >> 8)) cycles--;
                    }
                } break;




                /* ---------- COMPARISON ---------- */
                /* CMP */
                case INS_CMP_IM:        /* 2 cycles */
                {
                    int8_t value = FetchByte(cycles, memory);

                    CMPSetStatus(A - value);
                } break;
                case INS_CMP_ZP:        /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    int8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    CMPSetStatus(A - zeroPageValue);
                } break;
                case INS_CMP_ZPX:       /* 4 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
                    zeroPageAddress += X;
                    cycles--;

                    int8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    CMPSetStatus(A - zeroPageValue);
                } break;
                case INS_CMP_ABS:       /* 4 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    CMPSetStatus(A - AddressValue);
                } break;
                case INS_CMP_ABSX:      /* 4 (+1 if page crossed) */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint16_t TargetAddress = Address + X;

                    if ((Address && 0xFF00) != (TargetAddress && 0xFF00))
                    {
                        cycles--;
                    }

                    uint8_t AddressValue = ReadByte(cycles, TargetAddress, memory);

                    CMPSetStatus(A - AddressValue);
                } break;
                case INS_CMP_ABSY:      /* 4 (+1 if page crossed) */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint16_t TargetAddress = Address + Y;

                    if ((Address && 0xFF00) != (TargetAddress && 0xFF00))
                    {
                        cycles--;
                    }

                    uint8_t AddressValue = ReadByte(cycles, TargetAddress, memory);

                    CMPSetStatus(A - AddressValue);
                } break;
                case INS_CMP_IDX:       /* 6 cycles → low and high addresses are in the zero page */
                {
                    uint8_t Address = FetchByte(cycles, memory);

                    Address += X;       /* Carry discarded */
                    cycles--;

                    uint16_t AddressValue = ReadWord(cycles, Address, memory);
                    uint8_t Value = ReadByte(cycles, AddressValue, memory);

                    CMPSetStatus(A - Value);
                } break;
                case INS_CMP_IDY:       /* 5 cycles (+1 if page crossed) */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    uint16_t TargetAddress = Address + Y;      /* Carry not discarded*/

                    if ((Address && 0xFF00) != (TargetAddress && 0xFF00))
                    {
                        cycles--;
                    }

                    uint16_t AddressValue = ReadWord(cycles, Address, memory);
                    uint8_t Value = ReadByte(cycles, AddressValue, memory);

                    CMPSetStatus(A - Value);
                } break;

                /* CPX */
                case INS_CPX_IM:        /* 2 cycles */
                {
                    int8_t value = FetchByte(cycles, memory);

                    CMPSetStatus(X - value);
                } break;
                case INS_CPX_ZP:        /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    int8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    CMPSetStatus(X - zeroPageValue);
                } break;
                case INS_CPX_ABS:       /* 4 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    CMPSetStatus(X - AddressValue);
                } break;

                /* CPY */
                case INS_CPY_IM:        /* 2 cycles */
                {
                    int8_t value = FetchByte(cycles, memory);

                    CMPSetStatus(Y - value);
                } break;
                case INS_CPY_ZP:        /* 3 cycles */
                {
                    uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    int8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

                    CMPSetStatus(Y - zeroPageValue);
                } break;
                case INS_CPY_ABS:       /* 4 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    CMPSetStatus(Y - AddressValue);
                } break;




                /* ---------- JUMPS ---------- */
                case INS_JMP_ABS:       /* 3 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    PC = Address;
                } break;
                case INS_JMP_ID:        /* 5 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    uint16_t FinalAddress = ReadWord(cycles, Address, memory);
                    PC = FinalAddress;
                } break;
                case INS_JSR:       /* 6 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory); // 3

                    memory.WriteWord(PC, S, cycles); // 5
                    S -= 2 ;

                    PC = Address;
                    cycles--;
                } break;
                case INS_RTS:       /* 6 cycles */
                {
                    uint16_t ReturnAddress = ReadWord(cycles, S + 1, memory);
                    
                    S += 2;
                    cycles -= 2; // idk, not sure if the 2 cycles left are decremented here

                    PC = ReturnAddress;
                    cycles--;   
                } break;




                /* ---------- BREAK ---------- */
                /* Unfinished, supposed to consume 6 cycles */
                case INS_BRK:   /* 6 cycles */
                {
                    printf("\e%sEOF!\e[0m\n", GREEN);
                    return;
                } break;




                /* ---------- NOP ---------- */
                case INS_NOP:   /* 2 cycles */
                {
                    PC++;
                    cycles--;
                } break;




                /* ---------- ADC ---------- */
                case INS_ADC_IM:        /* 2 cycles */
                {
                    int8_t value = FetchByte(cycles, memory);

                    A += value + (P & 0b00000001);
                } break;
                case INS_ADC_ZP:        /* 3 cycles */
                {
                    
                } break;
                case INS_ADC_ZPX:        /* 4 cycles */
                {
                    
                } break;
                case INS_ADC_ABS:       /* 4 cycles */
                {
                    
                } break;
                case INS_ADC_ABSX:       /* 4 cycles (+1 if page crossed) */
                {
                    
                } break;
                case INS_ADC_ABSY:       /* 4 cycles (+1 if page crossed) */
                {
                    
                } break;
                case INS_ADC_IDX:       /* 6 cycles */
                {
                    
                } break;
                case INS_ADC_IDY:       /* 5 (+1 if page crossed) */
                {
                    
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

    uint8_t FetchByte(int32_t& cycles, Memory &memory) {
        uint8_t Data = memory[PC];
        PC++;
        cycles--;

        return Data;
    }

    /* Same as the previous function but doesn't increment the program counter */
    uint8_t ReadByte(int32_t& cycles, int32_t address, Memory &memory) {
        uint8_t Data = memory[address];
        printf("The uint8_t read is %04X\n", Data);
        cycles--;

        return Data;
    }

    /* TODO: adding a swap uint8_t function depending on whether the platform is little or big endian. */
    
    /* Also, I wanna mention that the "Word" here refers to 2 Bytes and does not refer to the CPU
       architecture which is 8-bit.
    */
    uint16_t FetchWord(int32_t& cycles, Memory &memory) {
        /* 6502 is little endian */
        uint16_t Data = memory[PC];         /* Lower uint8_t */
        PC++;

        Data |= (memory[PC] << 8);     /* Higher uint8_t */
        PC++;

        cycles -= 2;

        return Data;
    }

    uint16_t ReadWord(int32_t& cycles, int32_t address, Memory &memory) {
        /* 6502 is little endian */
        uint16_t Data = memory[address];            /* Lower uint8_t */
        printf("Address being read is: %04X, with value: %04X\n", address, Data);


        Data |= (memory[address + 1] << 8);     /* Higher uint8_t */
        printf("Address being read is: %04X, with value: %04X\n", address + 1, Data);

        cycles -= 2;

        return Data;
    }

    // opcodes
    static constexpr uint8_t
        /* ===== LOAD FROM MEMORY ===== */

        /* LDA → Loads a uint8_t of memory into the accumulator, setting zero and negative flags as appropriate */
        INS_LDA_IM = 0xA9,
        INS_LDA_ZP = 0xA5,
        INS_LDA_ZPX = 0xB5,
        INS_LDA_ABS = 0xAD,
        INS_LDA_ABSX = 0xBD,
        INS_LDA_ABSY = 0xB9,
        INS_LDA_IDX = 0xA1,
        INS_LDA_IDY = 0xB1,

        /* LDX → Loads a uint8_t of memory into the X register, setting the zero and negative flags as appropriate. */
        INS_LDX_IM = 0xA2,
        INS_LDX_ZP = 0xA6,
        INS_LDX_ZPY = 0xB6,
        INS_LDX_ABS = 0xAE,
        INS_LDX_ABSY = 0xBE,

        /* LDY → Loads a uint8_t of memory into the Y register, setting the zero and negative flags as appropriate. */
        INS_LDY_IM = 0xA0,
        INS_LDY_ZP = 0xA4,
        INS_LDY_ZPX = 0xB4,
        INS_LDY_ABS = 0xAC,
        INS_LDY_ABSX = 0xBC,

        /* ===== STORE TO MEMORY ===== */
        
        /* STA → Stores the contents of the accumulator into memory. */
        INS_STA_ZP = 0x85,
        INS_STA_ZPX = 0x95,
        INS_STA_ABS = 0x8D,
        INS_STA_ABSX = 0x9D,
        INS_STA_ABSY = 0x99,
        INS_STA_IDX = 0x81,
        INS_STA_IDY = 0x91,

        /* STX → Stores the contents of the X register into memory. */
        INS_STX_ZP = 0x86,
        INS_STX_ZPY = 0x96,
        INS_STX_ABS = 0x8E,

        /* STY → Stores the contents of the Y register into memory. */
        INS_STY_ZP = 0x84,
        INS_STY_ZPX = 0x94,
        INS_STY_ABS = 0x8C,

        /* ===== MOVE DATA BETWEEN REGISTERS ===== */
        /* TAX → Content from A to X register */
        INS_TAX = 0xAA,

        /* TAY → Content from A to Y register */
        INS_TAY = 0xA8,

        /* TXA → Content from X to A register */
        INS_TXA = 0x8A,

        /* TYA → Content from Y to A register */
        INS_TYA = 0x98,

        /* ===== INCREMENTING AND DECREMENTING ===== */
        /* DEX → Decrement X */
        INS_DEX = 0xCA,

        /* DEY → Decrement Y */
        INS_DEY = 0x88,
        
        /* INX → Increment X */
        INS_INX = 0xE8,
        
        /* INY → Increment Y */
        INS_INY = 0xC8,

        /* DEC → decrements the value held at a specific memory location */
        INS_DEC_ZP = 0xC6,
        INS_DEC_ZPX = 0xD6,
        INS_DEC_ABS = 0xCE,
        INS_DEC_ABSX = 0xDE,

        /* INC → increments the value held at a specific memory location */
        INS_INC_ZP = 0xE6,
        INS_INC_ZPX = 0xF6,
        INS_INC_ABS = 0xEE,
        INS_INC_ABSX = 0xFE,

        /* ===== BRANCHING ===== */
        INS_BNE = 0xD0,
        INS_BEQ = 0xF0,
        INS_BPL = 0x10,
        INS_BMI = 0x30,
        INS_BCC = 0x90,
        INS_BCS = 0xB0,
        INS_BVC = 0x50,
        INS_BVS = 0x70,

        /* ===== COMPARISON ===== */
        /*
         * Sets C, Z and N flags.
        */

        /* CMP → Compares the contents of the accumulator with another memory held value*/
        INS_CMP_IM = 0xC9,
        INS_CMP_ZP = 0xC5,
        INS_CMP_ZPX = 0xD5,
        INS_CMP_ABS = 0xCD,
        INS_CMP_ABSX = 0xDD,
        INS_CMP_ABSY = 0xD9,
        INS_CMP_IDX = 0xC1,
        INS_CMP_IDY = 0xD1,

        /* CPX → Compares the contents of the X register with another memory held value */
        INS_CPX_IM = 0xE0,
        INS_CPX_ZP = 0xE4,
        INS_CPX_ABS = 0xEC,
        
        /* CPY → Compares the contents of the Y register with another memory held value */
        INS_CPY_IM = 0xC0,
        INS_CPY_ZP = 0xC4,
        INS_CPY_ABS = 0xCC,

        /* ===== JUMPS ===== */
        /* JMP → Sets PC to the address specified by the operand. */
        INS_JMP_ABS = 0x4C,
        INS_JMP_ID = 0x6C,

        /* JSR → Pushes (address - 1) of the return point to the stack, and sets PC to the target memory address  */
        INS_JSR = 0x20,

        /* RTS → Pulls the program counter (minus one) from the stack. */
        INS_RTS = 0x60,

        /* ===== BREAK ===== */
        /* Unfinished */
        INS_BRK = 0x00,

        /* ===== NOP ===== */
        /* NOP → increments the program counter to the next instruction */
        INS_NOP = 0xEA,

        /* ===== ARITHMETIC OPERATIONS ===== */
        /* ADC → Adds memory to A with the carry bit, setting/clearing the overflow bit. */
        INS_ADC_IM = 0x69,
        INS_ADC_ZP = 0x65,
        INS_ADC_ZPX = 0x75,
        INS_ADC_ABS = 0x6D,
        INS_ADC_ABSX = 0x7D,
        INS_ADC_ABSY = 0x79,
        INS_ADC_IDX = 0x61,
        INS_ADC_IDY = 0x71;

    void LDSetStatus()
    {
        P |= ((A == 0) << 5);
        P |= (((A & 0b10000000) > 0) << 0);
    }

    void CMPSetStatus(uint8_t difference)
    {
        if (difference > 0)
        {
            P |= 0b00000001;
        } else if (difference == 0)  {
            P |= 0b00000011;
        } else {
            P |= 0b10000000;
        }
    }

    void Debug() {
        printf("A: %04X\nX: %04X\nY: %04X\nPC: %08X\nS: %08X\n", A, X, Y, PC, S);
    }
};