#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>

using namespace std;

// using uint8_t = unsigned char;     // 1 uint8_t
// using uint16_t = unsigned short;    // 2 bytes
// using uint32_t = unsigned int;       // 4 bytes

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
    static constexpr uint32_t MAX_MEMORY = 1024 * 64;
    static constexpr uint32_t ZERO_PAGE_SIZE = 256;
    uint8_t Data[MAX_MEMORY];

    void Initialize()
    {
        for (uint32_t i = 0; i < MAX_MEMORY; i++)
        {
            Data[i] = 0;
        }
    }

    void Debug(uint32_t start, uint32_t end)
    {
        int counter = 0;
        for (uint32_t i = start; i < end; i++)
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
        for (uint32_t i = 0; i < ZERO_PAGE_SIZE; i++)
        {
            uint32_t val = Data[i];
            
            if (val == 0)
                printf("%02X ", Data[i]);
            else
                printf("\e%s%02X\e[0m ", MAGENTA, Data[i]);

            if ((i + 1) % 8 == 0) cout << endl;
        }
    }

    /* read 1 uint8_t */
    uint8_t operator[](uint32_t address) const {
        return Data[address];
    }

    /* write 1 uint8_t */
    uint8_t& operator[](uint32_t address) {
        return Data[address];
    }

    /* write 2 bytes */
    uint16_t writeWord(uint16_t Value, uint32_t Address, uint32_t cycles) {
        Data[Address] = Value & 0xFF;
        Data[Address + 1] = (Value >> 8);

        cycles -= 2;
    }
};

struct CPU
{
    // Program Counter
    uint16_t PC;

    // Registers: Accumulator, Stack Pointer, Index Registers
    uint8_t A, S, X, Y;

    // Status register
    /*
     * [0] N → negative
     * [1] V → overflow
     * [2] B → break
     * [3] D → decimal
     * [4] I → interrupt disable
     * [5] unused, forced to 1 when something is pushed to the stack
     * [6] Z → zero
     * [7] C → carry
    */
    uint8_t P;

    void Reset(uint32_t cycles, Memory& memory)
    {
        uint16_t ResetVector = 0xFFFC;
        PC = ReadWord(cycles, ResetVector, memory);
        S = 0x0100;
        P = 0;
        A = X = Y = 0;
    }
    
    /* Insert program into memory */
    void MountProgram(vector<uint8_t> instructions, Memory& memory) {
        uint32_t StartAddress = PC;
        
        for (uint8_t ins : instructions) {
            memory[StartAddress] = ins;
            StartAddress++; 
        }
    }

    void Execute(uint32_t cycles, Memory& memory)
    {
        while (cycles > 0) {
            uint8_t instruction = FetchByte(cycles, memory);
            printf("The current instruction is %04X\n", instruction);
            
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
                    uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
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
                    uint8_t Address = FetchWord(cycles, memory);
                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;
                    LDSetStatus();
                } break;
                case INS_LDA_ABSX:  /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t Address = FetchWord(cycles, memory);
                    Address += X;
                    cycles--;

                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;

                    LDSetStatus();
                } break;
                case INS_LDA_ABSY:  /* 4 cycles (+1 if page crossed) */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    Address += Y;
                    cycles--;

                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

                    A = AddressValue;

                    LDSetStatus();
                } break;
                case INS_LDA_INDX:  /* 6 cycles */
                {
                    // LDA ($40,X)
                    uint8_t Address = FetchByte(cycles, memory);

                    Address += X;
                    cycles--;

                    uint16_t AddressValue = ReadWord(cycles, Address, memory);
                    A = memory[AddressValue];

                    LDSetStatus();
                } break;
                case INS_LDA_INDY:  /* 5 cycles (+1 if page crossed) */
                {
                    // LDA ($40),Y
                    uint8_t Address = FetchByte(cycles, memory);
                    uint16_t AddressValue = ReadWord(cycles, Address, memory);

                    AddressValue += Y;
                    cycles--;

                    uint16_t FinalValue = ReadWord(cycles, AddressValue, memory);
                    A = FinalValue;

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
                    Address += Y;
                    cycles--;

                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

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
                    Address += X;
                    cycles--;

                    uint8_t AddressValue = ReadByte(cycles, Address, memory);

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
                case INS_STA_INDX:
                {
                    // STA ($40,X)
                    uint8_t Address = FetchWord(cycles, memory);

                    Address += X;
                    cycles--;

                    uint16_t AddressValue = ReadWord(cycles, Address, memory);

                    memory[AddressValue] = A;
                } break;
                case INS_STA_INDY:
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
                default:
                {
                    /* TODO: Exception object */
                    printf("Instruction not handled %d", instruction);

                    cycles--;
                } break;
            }
        }
    }

    uint8_t FetchByte(uint32_t& cycles, Memory &memory) {
        uint8_t Data = memory[PC];
        PC++;
        cycles--;

        return Data;
    }

    /* Same as the previous function but doesn't increment the program counter */
    uint8_t ReadByte(uint32_t& cycles, uint32_t address, Memory &memory) {
        uint8_t Data = memory[address];
        printf("The uint8_t read is %04X\n", Data);
        cycles--;

        return Data;
    }

    /* TODO: adding a swap uint8_t function depending on whether the platform is little or big endian. */
    uint16_t FetchWord(uint32_t& cycles, Memory &memory) {
        /* 6502 is little endian */
        uint16_t Data = memory[PC];         /* Lower uint8_t */
        PC++;

        Data |= (memory[PC] << 8);     /* Higher uint8_t */
        PC++;

        cycles -= 2;

        return Data;
    }

    uint16_t ReadWord(uint32_t& cycles, uint32_t address, Memory &memory) {
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
        INS_LDA_INDX = 0xA1,
        INS_LDA_INDY = 0xB1,

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
        INS_STA_INDX = 0x81,
        INS_STA_INDY = 0x91,

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
        INS_INC_ABSX = 0xFE;

    void LDSetStatus()
    {
        P |= ((A == 0) << 5);
        P |= (((A & 0b10000000) > 0) << 0);
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
    vector<uint8_t> instructions = {
        CPU::INS_LDA_IM, 0x30,  // 2
        CPU::INS_LDX_IM, 0x50,  // 2
        CPU::INS_STA_ZP, 0x57,  // 3
        CPU::INS_STX_ZP, 0x58,  // 3
        CPU::INS_DEC_ZP, 0x58,  // 5
        CPU::INS_INC_ZP, 0x57   // 5
    };

    /* Point the Reset Vector (0xFFFC/0xFFFD) to the program's start address */
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x80;

    /* Initializing CPU */
    CPU cpu;
    cpu.Reset(2, memory);
    cpu.MountProgram(instructions, memory);
    cpu.X = 0x05;

    /* Execution */
    cpu.Execute(20, memory);

    /* Debugging */
    memory.Debug(0xFFF0, 0xFFFF);
    memory.Debug(0x1555, 0x1565);
    cpu.Debug();
    memory.DebugZeroPage();

    return 0;
}
