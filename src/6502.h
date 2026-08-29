#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <bitset>

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

uint16_t STACK_ADDRESS = 0x0100;
uint16_t STACK_ADDRESS_END = 0x01FF;

uint16_t ZERO_PAGE = 0x0100;
uint16_t ZERO_PAGE_END = 0x01FF;

uint16_t RESET_VECTOR = 0xFFFC;

/* Status bits positions */
#define CARRY_BIT 0
#define ZERO_BIT 1
#define INTERRUPT_DISABLE_BIT 2
#define DECIMAL_BIT 3
#define BREAK_BIT 4
#define OVERFLOW_BIT 6
#define NEGATIVE_BIT 7

struct Memory
{
    static constexpr uint32_t MAX_MEMORY = 1024 * 64;
    static constexpr uint8_t PAGE_SIZE = 0xFF;
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

        for (uint32_t i = PageStart; i <= PageBoundary; i++)
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

    /* write 1 byte */
    void WriteByte(uint8_t Value, uint32_t address, uint32_t &cycles) {
        Data[address] = Value;

        cycles--;
    }
    
    /* write 2 bytes */
    void WriteWord(uint16_t Value, uint32_t address, uint32_t &cycles) {
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
    uint8_t S;

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

    void Reset(uint32_t cycles, Memory& memory)
    {
        uint16_t ResetVector = 0xFFFC;
        PC = ReadWord(cycles, ResetVector, memory);
        S = 0xFF;
        P = 0;
        A = X = Y = 0;
    }
    
    /* Insert program into memory */
    void MountProgram(vector<uint32_t> instructions, Memory& memory, uint32_t cycles) {
        uint32_t StartAddress = PC;
        
        for (uint8_t ins : instructions) {
            if (cycles > 0) {
                memory[StartAddress] = ins;
                StartAddress++;
                cycles--;
            }
        }
    }

    /* Return the number of cycles that were consumed */
    uint32_t Execute(uint32_t cycles, Memory& memory)
    {
        uint32_t TotalCycles = cycles;
        while (cycles > 0) {
            uint8_t instruction = FetchByte(cycles, memory);
            
            switch (instruction)
            {
                /* -------------------- LDA -------------------- */
                case INS_LDA_IM:    /* 2 cycles */
                {
                    A = GetImmediate(cycles, memory);

                    LDSetStatus(A);
                } break;
                case INS_LDA_ZP:    /* 3 cycles */
                {
                    A = GetZeroPage(cycles, memory);

                    LDSetStatus(A);
                } break;
                case INS_LDA_ZPX:   /* 4 cycles */
                {
                    A = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    LDSetStatus(A);
                } break;
                case INS_LDA_ABS:   /* 4 cycles */
                {
                    A = GetAbsolute(cycles, memory);

                    LDSetStatus(A);
                } break;
                case INS_LDA_ABSX:  /* 4 cycles (+1 if page crossed) */
                {
                    A = GetAbsoluteX(cycles, memory);

                    LDSetStatus(A);
                } break;
                case INS_LDA_ABSY:  /* 4 cycles (+1 if page crossed) */
                {
                    A = GetAbsoluteY(cycles, memory);

                    LDSetStatus(A);
                } break;
                case INS_LDA_IDX:  /* 6 cycles */
                {
                    A = GetIndirectX(cycles, memory);

                    LDSetStatus(A);
                } break;
                case INS_LDA_IDY:  /* 5 cycles (+1 if page crossed) */
                {
                    A = GetIndirectY(cycles, memory);

                    LDSetStatus(A);
                } break;




                /* -------------------- LDX -------------------- */
                case INS_LDX_IM:    /* 2 cycles */
                {
                    X = GetImmediate(cycles, memory);

                    LDSetStatus(X);
                } break;
                case INS_LDX_ZP:    /* 3 cycles */
                {
                    X = GetZeroPage(cycles, memory);;

                    LDSetStatus(X);
                } break;
                case INS_LDX_ZPY:   /* 4 cycles */
                {
                    X = GetZeroPageY(cycles, memory, TotalCycles);
                    
                    LDSetStatus(X);
                } break;
                case INS_LDX_ABS:   /* 4 cycles */
                {
                    X = GetAbsolute(cycles, memory);
                    LDSetStatus(X);
                } break;
                case INS_LDX_ABSY:  /* 4 cycles (+1 if page crossed) */
                {
                    X = GetAbsoluteY(cycles, memory);;

                    LDSetStatus(X);
                } break;




                /* -------------------- LDY -------------------- */
                case INS_LDY_IM:    /* 2 cycles */
                {
                    Y = GetImmediate(cycles, memory);

                    LDSetStatus(Y);
                } break;
                case INS_LDY_ZP:    /* 3 cycles */
                {
                    Y = GetZeroPage(cycles, memory);

                    LDSetStatus(Y);
                } break;
                case INS_LDY_ZPX:   /* 4 cycles */
                {
                    Y = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    LDSetStatus(Y);
                } break;
                case INS_LDY_ABS:   /* 4 cycles */
                {
                    Y = GetAbsolute(cycles, memory);
                    LDSetStatus(Y);
                } break;
                case INS_LDY_ABSX:  /* 4 cycles (+1 if page crossed) */
                {
                    Y = GetAbsoluteX(cycles, memory);;

                    LDSetStatus(Y);
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
                case INS_STA_IDX:      /* 6 cycles */
                {
                    // STA ($40,X)
                    uint8_t Address = FetchByte(cycles, memory);

                    Address += X;
                    cycles--;

                    // Wraps around 0 page
                    uint16_t AddressValue = memory[Address] | (memory[(uint8_t) (Address + 1)] << 8);
                    cycles -= 2; 

                    memory[AddressValue] = A;
                    cycles--;
                } break;
                case INS_STA_IDY:      /* 6 cycles */
                {
                    // STA ($40),Y
                    uint8_t Address = FetchByte(cycles, memory);
                    // Wraps around 0 page
                    uint16_t AddressValue = memory[Address] | (memory[(uint8_t) (Address + 1)] << 8);
                    cycles -= 2; 

                    uint16_t TargetAddress = AddressValue + Y;
                    cycles--;

                    uint8_t FinalValue =  memory[TargetAddress];
                    cycles--;

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
                    LDSetStatus(X);
                } break;
                case INS_TAY:       /* 2 cycles */
                {
                    Y = A;
                    cycles--;
                    LDSetStatus(Y);
                } break;
                case INS_TXA:       /* 2 cycles */
                {
                    A = X;
                    cycles--;
                    LDSetStatus(A);
                } break;
                case INS_TYA:       /* 2 cycles */
                {
                    A = Y;
                    cycles--;
                    LDSetStatus(A);
                } break;




                /* ---------- INCREMENTING AND DECREMENTING REGISTERS ---------- */
                case INS_DEX:       /* 2 cycles */
                {
                    X--;
                    cycles--;
                    LDSetStatus(X);
                } break;
                case INS_DEY:       /* 2 cycles */
                {
                    Y--;
                    cycles--;
                    LDSetStatus(Y);
                } break;
                case INS_INX:       /* 2 cycles */
                {
                    X++;
                    cycles--;
                    LDSetStatus(X);
                } break;
                case INS_INY:       /* 2 cycles */
                {
                    Y++;
                    cycles--;
                    LDSetStatus(Y);
                } break;




                /* ---------- INCREMENTING AND DECREMENTING A MEMORY VALUE ---------- */
                case INS_DEC_ZP:        /* 5 cycles */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    uint8_t Value = ReadByte(cycles, Address, memory);
                    
                    Value--;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;

                    SetStatusBit(ZERO_BIT, Value == 0);

                    SetStatusBit(NEGATIVE_BIT, (Value & 0x80) >> 7);
                } break;
                case INS_DEC_ZPX:       /* 6 cycles */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    Address += X;
                    cycles--;
                    
                    uint8_t Value = ReadByte(cycles, Address, memory);
                    
                    Value--;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;

                    SetStatusBit(ZERO_BIT, Value == 0);

                    SetStatusBit(NEGATIVE_BIT, (Value & 0x80) >> 7);
                } break;
                case INS_DEC_ABS:       /* 6 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    
                    uint8_t Value = ReadByte(cycles, Address, memory);
                    
                    Value--;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;

                    SetStatusBit(ZERO_BIT, Value == 0);

                    SetStatusBit(NEGATIVE_BIT, (Value & 0x80) >> 7);
                } break;
                case INS_DEC_ABSX:      /* 7 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    Address += X;
                    cycles--;
                    
                    uint8_t Value = ReadByte(cycles, Address, memory);
                    
                    Value--;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;

                    SetStatusBit(ZERO_BIT, Value == 0);

                    SetStatusBit(NEGATIVE_BIT, (Value & 0x80) >> 7);
                } break;

                case INS_INC_ZP:        /* 5 cycles */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    
                    uint8_t Value = ReadByte(cycles, Address, memory);
                    
                    Value++;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;

                    SetStatusBit(ZERO_BIT, Value == 0);

                    SetStatusBit(NEGATIVE_BIT, (Value & 0x80) >> 7);
                } break;
                case INS_INC_ZPX:       /* 6 cycles */
                {
                    uint8_t Address = FetchByte(cycles, memory);
                    Address += X;
                    cycles--;
                    
                    uint8_t Value = ReadByte(cycles, Address, memory);
                    
                    Value++;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;

                    SetStatusBit(ZERO_BIT, Value == 0);

                    SetStatusBit(NEGATIVE_BIT, (Value & 0x80) >> 7);
                } break;
                case INS_INC_ABS:       /* 6 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    
                    uint8_t Value = ReadByte(cycles, Address, memory);
                    
                    Value++;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;

                    SetStatusBit(ZERO_BIT, Value == 0);

                    SetStatusBit(NEGATIVE_BIT, (Value & 0x80) >> 7);
                } break;
                case INS_INC_ABSX:      /* 7 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory);
                    Address += X;
                    cycles--;
                    
                    uint8_t Value = ReadByte(cycles, Address, memory);
                    
                    Value++;
                    cycles--;
                    
                    memory[Address] = Value;
                    cycles--;

                    SetStatusBit(ZERO_BIT, Value == 0);

                    SetStatusBit(NEGATIVE_BIT, (Value & 0x80) >> 7);
                } break;




                /* ---------- BRANCHING ---------- */
                case INS_BNE:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    /* If the zero flag is clear */
                    if (GetStatusBit(ZERO_BIT) == 0) {
                        Branch(cycles, memory);
                    } else {
                        PC++;
                    }
                } break;
                case INS_BEQ:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    /* If the zero flag is set */
                    if (GetStatusBit(ZERO_BIT) == 1) {
                        Branch(cycles, memory);
                    } else {
                        PC++;
                    }
                } break;
                case INS_BPL:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    /* If negative flag is clear */
                    if (GetStatusBit(NEGATIVE_BIT) == 0) {
                        Branch(cycles, memory);
                    } else {
                        PC++;
                    }
                } break;
                case INS_BMI:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    /* If negative flag is set */
                    if (GetStatusBit(NEGATIVE_BIT) == 1) {
                        Branch(cycles, memory);
                    } else {
                        PC++;
                    }
                } break;
                case INS_BCC:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    /* If carry flag is clear */
                    if (GetStatusBit(CARRY_BIT) == 0) {
                        Branch(cycles, memory);
                    } else {
                        PC++;
                    }
                } break;
                case INS_BCS:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    /* If carry flag is set */
                    if (GetStatusBit(CARRY_BIT) == 1) {
                        Branch(cycles, memory);
                    } else {
                        PC++;
                    }
                } break;
                case INS_BVC:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    /* If overflow flag is clear */
                    if (GetStatusBit(OVERFLOW_BIT) == 0) {
                        Branch(cycles, memory);
                    } else {
                        PC++;
                    }
                } break;
                case INS_BVS:   // 2 cycles (+1 if branch succeeds, +2 if to a new page)
                {
                    /* If overflow flag is set */
                    if (GetStatusBit(OVERFLOW_BIT) == 1) {
                        Branch(cycles, memory);
                    } else {
                        PC++;
                    }
                } break;




                /* ---------- COMPARISON ---------- */
                /* CMP */
                case INS_CMP_IM:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    CMPSetStatus(value, A);
                } break;
                case INS_CMP_ZP:        /* 3 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    CMPSetStatus(value, A);
                } break;
                case INS_CMP_ZPX:       /* 4 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);

                    CMPSetStatus(value, A);
                } break;
                case INS_CMP_ABS:       /* 4 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    CMPSetStatus(value, A);
                } break;
                case INS_CMP_ABSX:      /* 4 (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    CMPSetStatus(value, A);
                } break;
                case INS_CMP_ABSY:      /* 4 (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteY(cycles, memory);

                    CMPSetStatus(value, A);
                } break;
                case INS_CMP_IDX:       /* 6 cycles → low and high addresses are in the zero page */
                {
                    uint8_t value = GetIndirectX(cycles, memory);

                    CMPSetStatus(value, A);
                } break;
                case INS_CMP_IDY:       /* 5 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetIndirectY(cycles, memory);

                    CMPSetStatus(value, A);
                } break;

                /* CPX */
                case INS_CPX_IM:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    CMPSetStatus(value, X);
                } break;
                case INS_CPX_ZP:        /* 3 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    CMPSetStatus(value, X);
                } break;
                case INS_CPX_ABS:       /* 4 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    CMPSetStatus(value, X);
                } break;

                /* CPY */
                case INS_CPY_IM:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    CMPSetStatus(value, Y);
                } break;
                case INS_CPY_ZP:        /* 3 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    CMPSetStatus(value, Y);
                } break;
                case INS_CPY_ABS:       /* 4 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    CMPSetStatus(value, Y);
                } break;




                /* ---------- JUMPS ---------- */

                /* JMP */
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

                /* JSR */
                case INS_JSR:       /* 6 cycles */
                {
                    uint16_t Address = FetchWord(cycles, memory); // 3

                    memory.WriteWord(PC, 0x100 | S, cycles); // 5
                    S = (uint8_t) (S - 2);

                    PC = Address;
                    cycles--;
                } break;

                /* RTS */
                case INS_RTS:       /* 6 cycles */
                {
                    uint16_t ReturnAddress = ReadWord(cycles, 0x100 | (S + 1), memory);
                    
                    S = (uint8_t) (S + 2);
                    cycles--; // +1 cycle to increment the stack pointer

                    PC = ReturnAddress + 1;
                    cycles--;

                    // 1-byte instructions consume an extra cycle.
                    cycles--;
                } break;

                /* RTI */
                case INS_RTI:       /* 6 cycles */
                {
                    uint8_t flag = ReadByte(cycles, 0x100 | (S + 1), memory);

                    uint16_t ReturnAddress = ReadWord(cycles, 0x100 | (S + 2), memory);

                    S = (uint8_t) (S + 3);
                    cycles--; // +1 cycle to increment the stack pointer

                    PC = ReturnAddress;
                    P = flag;

                    // 1-byte instructions consume an extra cycle.
                    cycles--;
                } break;




                /* ---------- BREAK ---------- */
                /* The BRK instruction forces the generation of an interrupt request. */
                case INS_BRK:   /* 7 cycles */
                {   
                    /* The program counter and processor status are pushed on the stack */
                    memory.WriteWord(PC, 0x100 | S, cycles);    // 3
                    S -= 2;

                    memory.WriteByte(P, 0x100 | S, cycles);     // 4
                    S--;

                    /* The content of the interrupt vector at $FFFE/F is loaded into the PC */ 
                    uint16_t interrupt_content = ReadWord(cycles, 0xFFFE, memory); // 6
                    PC = interrupt_content;
                    
                    /* The break flag in the status set to one. */
                    SetStatusBit(BREAK_BIT, 1);

                    printf("\e%sEOF!\e[0m\n", GREEN);

                    /* Cycle for the 1-byte instruction penaltyf */
                    cycles--;       // 7
                    return TotalCycles - cycles;
                } break;




                /* ---------- NOP ---------- */
                case INS_NOP:   /* 2 cycles */
                {
                    cycles--;
                } break;




                /* ---------- BIT ---------- */
                case INS_BIT_ZP:   /* 3 cycles */
                {
                    A &= GetZeroPage(cycles, memory);

                    SetStatusBit(NEGATIVE_BIT, (A & 0x80) >> 7);

                    SetStatusBit(OVERFLOW_BIT, (A & 0x40) >> 6);
                } break;
                case INS_BIT_ABS:   /* 4 cycles */
                {
                    A &= GetAbsolute(cycles, memory);

                    SetStatusBit(NEGATIVE_BIT, (A & 0x80) >> 7);

                    SetStatusBit(OVERFLOW_BIT, (A & 0x40) >> 6);
                } break;


                /* ---------- ADC ---------- */
                case INS_ADC_IM:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    ADC(value);
                } break;
                case INS_ADC_ZP:        /* 3 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    ADC(value);
                } break;
                case INS_ADC_ZPX:        /* 4 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    ADC(value);
                } break;
                case INS_ADC_ABS:       /* 4 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    ADC(value);
                } break;
                case INS_ADC_ABSX:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    ADC(value);
                } break;
                case INS_ADC_ABSY:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteY(cycles, memory);

                    ADC(value);
                } break;
                case INS_ADC_IDX:       /* 6 cycles */
                {
                    uint8_t value = GetIndirectX(cycles, memory);

                    ADC(value);
                } break;
                case INS_ADC_IDY:       /* 5 (+1 if page crossed) */
                {
                    uint8_t value = GetIndirectY(cycles, memory);

                    ADC(value);
                } break;

                /* ---------- SBC ---------- */
                case INS_SBC_IM:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    ADC(~value);
                } break;
                case INS_SBC_ZP:        /* 3 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    ADC(~value);
                } break;
                case INS_SBC_ZPX:        /* 4 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    ADC(~value);
                } break;
                case INS_SBC_ABS:       /* 4 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    ADC(~value);
                } break;
                case INS_SBC_ABSX:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    ADC(~value);
                } break;
                case INS_SBC_ABSY:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteY(cycles, memory);

                    ADC(~value);
                } break;
                case INS_SBC_IDX:       /* 6 cycles */
                {
                    uint8_t value = GetIndirectX(cycles, memory);

                    ADC(~value);
                } break;
                case INS_SBC_IDY:       /* 5 (+1 if page crossed) */
                {
                    uint8_t value = GetIndirectY(cycles, memory);

                    ADC(~value);
                } break;




                /* ---------- PSR OPERATIONS ---------- */
                case INS_CLC:   /* 2 cycles */
                {
                    SetStatusBit(CARRY_BIT, 0);
                    cycles--;
                } break;
                case INS_SEC:   /* 2 cycles */
                {
                    SetStatusBit(CARRY_BIT, 1);
                    cycles--;
                } break;
                case INS_CLV:   /* 2 cycles */
                {
                    SetStatusBit(OVERFLOW_BIT, 0);
                    cycles--;
                } break;
                case INS_SED:   /* 2 cycles */
                {
                    SetStatusBit(DECIMAL_BIT, 1);
                    cycles--;
                } break;
                case INS_CLD:   /* 2 cycles */
                {
                    SetStatusBit(DECIMAL_BIT, 0);
                    cycles--;
                } break;
                case INS_CLI:   /* 2 cycles */
                {
                    SetStatusBit(INTERRUPT_DISABLE_BIT, 0);
                    cycles--;
                }
                case INS_SEI:   /* 2 cycles */
                {
                    SetStatusBit(INTERRUPT_DISABLE_BIT, 1);
                    cycles--;
                }




                /* ---------- LOGICAL OPERATIONS ---------- */
                /* AND */
                case INS_AND_IM:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    AND(value);
                } break;
                case INS_AND_ZP:        /* 3 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    AND(value);
                } break;
                case INS_AND_ZPX:        /* 4 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    AND(value);
                } break;
                case INS_AND_ABS:       /* 4 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    AND(value);
                } break;
                case INS_AND_ABSX:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    AND(value);
                } break;
                case INS_AND_ABSY:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteY(cycles, memory);

                    AND(value);
                } break;
                case INS_AND_IDX:       /* 6 cycles */
                {
                    uint8_t value = GetIndirectX(cycles, memory);

                    AND(value);
                } break;
                case INS_AND_IDY:       /* 5 (+1 if page crossed) */
                {
                    uint8_t value = GetIndirectY(cycles, memory);

                    AND(value);
                } break;

                
                /* ORA */
                case INS_ORA_IM:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    ORA(value);
                } break;
                case INS_ORA_ZP:        /* 3 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    ORA(value);
                } break;
                case INS_ORA_ZPX:        /* 4 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    ORA(value);
                } break;
                case INS_ORA_ABS:       /* 4 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    ORA(value);
                } break;
                case INS_ORA_ABSX:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    ORA(value);
                } break;
                case INS_ORA_ABSY:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteY(cycles, memory);

                    ORA(value);
                } break;
                case INS_ORA_IDX:       /* 6 cycles */
                {
                    uint8_t value = GetIndirectX(cycles, memory);

                    ORA(value);
                } break;
                case INS_ORA_IDY:       /* 5 (+1 if page crossed) */
                {
                    uint8_t value = GetIndirectY(cycles, memory);

                    ORA(value);
                } break;

                
                /* EOR */
                case INS_EOR_IM:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    EOR(value);
                } break;
                case INS_EOR_ZP:        /* 3 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    EOR(value);
                } break;
                case INS_EOR_ZPX:        /* 4 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    EOR(value);
                } break;
                case INS_EOR_ABS:       /* 4 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    EOR(value);
                } break;
                case INS_EOR_ABSX:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    EOR(value);
                } break;
                case INS_EOR_ABSY:       /* 4 cycles (+1 if page crossed) */
                {
                    uint8_t value = GetAbsoluteY(cycles, memory);

                    EOR(value);
                } break;
                case INS_EOR_IDX:       /* 6 cycles */
                {
                    uint8_t value = GetIndirectX(cycles, memory);

                    EOR(value);
                } break;
                case INS_EOR_IDY:       /* 5 (+1 if page crossed) */
                {
                    uint8_t value = GetIndirectY(cycles, memory);

                    EOR(value);
                } break;




                /* ---------- ROTATION AND SHIFT OPERATIONS ---------- */
                /* ASL */
                case INS_ASL_A:        /* 2 cycles */
                {

                    ASL(A);
                } break;
                case INS_ASL_ZP:        /* 5 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    ASL(value);
                } break;
                case INS_ASL_ZPX:        /* 6 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    ASL(value);
                } break;
                case INS_ASL_ABS:       /* 6 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    ASL(value);
                } break;
                case INS_ASL_ABSX:       /* 7 cycles */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    ASL(value);
                } break;

                /* LSR */
                case INS_LSR_A:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    LSR(value);
                } break;
                case INS_LSR_ZP:        /* 5 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    LSR(value);
                } break;
                case INS_LSR_ZPX:        /* 6 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    LSR(value);
                } break;
                case INS_LSR_ABS:       /* 6 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    LSR(value);
                } break;
                case INS_LSR_ABSX:       /* 7 cycles */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    LSR(value);
                } break;

                /* ROL */
                case INS_ROL_A:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    ROL(value);
                } break;
            case INS_ROL_ZP:        /* 5 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    ROL(value);
                } break;
                case INS_ROL_ZPX:        /* 6 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    ROL(value);
                } break;
                case INS_ROL_ABS:       /* 6 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    ROL(value);
                } break;
                case INS_ROL_ABSX:       /* 7 cycles */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    ROL(value);
                } break;

                /* ROR */
                case INS_ROR_A:        /* 2 cycles */
                {
                    uint8_t value = GetImmediate(cycles, memory);

                    ROR(value);
                } break;
                case INS_ROR_ZP:        /* 5 cycles */
                {
                    uint8_t value = GetZeroPage(cycles, memory);

                    ROR(value);
                } break;
                case INS_ROR_ZPX:        /* 6 cycles */
                {
                    uint8_t value = GetZeroPageX(cycles, memory, TotalCycles);
                    
                    ROR(value);
                } break;
                case INS_ROR_ABS:       /* 6 cycles */
                {
                    uint8_t value = GetAbsolute(cycles, memory);

                    ROR(value);
                } break;
                case INS_ROR_ABSX:       /* 7 cycles */
                {
                    uint8_t value = GetAbsoluteX(cycles, memory);

                    ROR(value);
                } break;




                /* ---------- STACK OPERATIONS ---------- */
                case INS_TXS:   /* 2 cycles */
                {
                    S &= (X | 0x100);
                    cycles--;
                } break;
                case INS_TSX:   /* 2 cycles */
                {
                    X = S;
                    
                    SetStatusBit(ZERO_BIT, X == 0);

                    SetStatusBit(NEGATIVE_BIT, (X & 0x80) >> 7);

                    // 1-byte instructions consume an extra cycle.
                    cycles--;
                } break;
                case INS_PHA:   /* 3 cycles */
                {
                    memory[S] = A;
                    cycles--;

                    S = (uint16_t) 0x100 | ((uint8_t) (S - 1));
                    
                    // 1-byte instructions consume an extra cycle.
                    cycles--;
                } break;
                case INS_PLA:   /* 4 cycles */
                {
                    S = (uint16_t) 0x100 | ((uint8_t) (S + 1));
                    cycles--;

                    A = memory[S];
                    cycles--;

                    SetStatusBit(ZERO_BIT, A == 0);

                    SetStatusBit(NEGATIVE_BIT, (A & 0x80) >> 7);

                    // 1-byte instructions consume an extra cycle.
                    cycles--;
                } break;
                case INS_PHP:   /* 3 cycles */
                {
                    memory[S] = P;
                    cycles--;

                    S = (uint16_t) 0x100 | ((uint8_t) (S - 1));
                    
                    // 1-byte instructions consume an extra cycle.
                    cycles--;
                } break;
                case INS_PLP:   /* 4 cycles */
                {
                    S = (uint16_t) 0x100 | ((uint8_t) (S + 1));
                    cycles--;

                    P = memory[S];
                    cycles--;

                    // 1-byte instructions consume an extra cycle.
                    cycles--;
                } break;



                default:
                {
                    /* TODO: Exception object */
                    printf("Instruction not handled %02x\n", instruction);

                    cycles--;

                    return TotalCycles - cycles;
                } break;
            }
        }

        printf("Total cycles are %d, cycles left are %d\n", TotalCycles, cycles);

        return TotalCycles - cycles;
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
        cycles--;

        return Data;
    }

    /* TODO: adding a swap uint8_t function depending on whether the platform is little or big endian. */
    
    /* Also, I wanna mention that the "Word" here refers to 2 Bytes and does not refer to the CPU
       architecture which is 8-bit.
    */
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

        Data |= (memory[address + 1] << 8);     /* Higher uint8_t */

        cycles -= 2;

        return Data;
    }

    /* Status Bit Operations */
    uint8_t GetStatusBit(uint32_t position)
    {

        return (P >> position) & 0b00000001;
    }

    void SetStatusBit(uint32_t position, uint8_t bitToWrite)
    {

        if (bitToWrite == 0)
        {
            P &= (~(!bitToWrite << position));
        }


        if (bitToWrite == 1)
        {
            P |= (bitToWrite << position);
        }
    }

    /* Addressing Modes Helpers */
    uint8_t GetImmediate(uint32_t &cycles, Memory &memory)
    {
        return FetchByte(cycles, memory);
    }

    uint8_t GetZeroPage(uint32_t &cycles, Memory &memory)
    {
        uint8_t zeroPageAddress = FetchByte(cycles, memory);
        uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

        return zeroPageValue;
    }

    uint8_t GetZeroPageX(uint32_t &cycles, Memory &memory, uint32_t TotalCycles)
    {
        uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
        zeroPageAddress += X;

        if (zeroPageAddress > ZERO_PAGE_END) {
            return TotalCycles - cycles;
        }

        cycles--;

        uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);
    
        return zeroPageValue;
    }

    uint8_t GetZeroPageY(uint32_t &cycles, Memory &memory, uint32_t TotalCycles)
    {
        uint8_t zeroPageAddress = FetchByte(cycles, memory);
                    
        zeroPageAddress += Y;

        if (zeroPageAddress > ZERO_PAGE_END) {
            return TotalCycles - cycles;
        }

        cycles--;

        uint8_t zeroPageValue = ReadByte(cycles, zeroPageAddress, memory);

        return zeroPageValue;
    }

    uint8_t GetAbsolute(uint32_t &cycles, Memory &memory)
    {
        uint16_t Address = FetchWord(cycles, memory);
        uint8_t AddressValue = ReadByte(cycles, Address, memory);

        return AddressValue;
    }

    uint8_t GetAbsoluteX(uint32_t &cycles, Memory &memory)
    {
        uint16_t Address = FetchWord(cycles, memory);
        uint16_t TargetAddress = Address + X;

        if ((Address & 0xFF00) != (TargetAddress & 0xFF00))
        {
            cycles--;
        }

        uint8_t AddressValue = ReadByte(cycles, TargetAddress, memory);

        return AddressValue;
    }

    uint8_t GetAbsoluteY(uint32_t &cycles, Memory &memory)
    {
        uint16_t Address = FetchWord(cycles, memory);
        uint16_t TargetAddress = Address + Y;

        if ((Address & 0xFF00) != (TargetAddress & 0xFF00))
        {
            cycles--;
        }

        uint8_t AddressValue = ReadByte(cycles, TargetAddress, memory);

        return AddressValue;
    }

    uint8_t GetIndirectX(uint32_t &cycles, Memory &memory)
    {
        // OPCODE ($40,X)
        uint8_t Address = FetchByte(cycles, memory);

        Address += X;
        printf("Address + X is: %d \n", Address);

        cycles--;

        // Wraps around 0 page
        uint16_t AddressValue = memory[Address] | (memory[(uint8_t) (Address + 1)] << 8);
        cycles -= 2; 

        uint8_t value = memory[AddressValue];
        cycles--;

        return value;
    }

    uint8_t GetIndirectY(uint32_t &cycles, Memory &memory)
    {
        // OPCODE ($40),Y
        uint8_t Address = FetchByte(cycles, memory);

        // Wraps around 0 page
        uint16_t AddressValue = memory[Address] | (memory[(uint8_t) (Address + 1)] << 8);
        cycles -= 2; 

        uint16_t TargetAddress = AddressValue + Y;

        if ((AddressValue & 0xFF00) != (TargetAddress & 0xFF00))
        {
            cycles--;
        }

        uint8_t value = memory[TargetAddress];
        cycles--;

        return value;
    }

    /* Repetitive Operations */
    void ADC(uint8_t value)
    {
        uint16_t sum;
        uint8_t carry_bit = GetStatusBit(CARRY_BIT);

        if (GetStatusBit(DECIMAL_BIT))
        {
            sum = BCD(BCD(A, value), carry_bit);
        } else
        {
            sum = A + value + carry_bit;
        }
                    
        uint8_t carry = sum > 0xFF;

        uint8_t overflow = (((A ^ sum) & (value ^ sum) & 0b10000000) != 0);

        A = sum;

        /* Carry flag */
        SetStatusBit(CARRY_BIT, carry);

        /* Zero Flag */
        SetStatusBit(ZERO_BIT, A == 0);

        /* Overflow flag */
        SetStatusBit(OVERFLOW_BIT, overflow);

        /* Negative flag */
        SetStatusBit(NEGATIVE_BIT, (A & 0b10000000) > 0);
    }

    void AND(uint8_t value)
    {
        A &= value;

        LogicalSetStatus(A);
    }

    void ORA(uint8_t value)
    {
        A |= value;

        LogicalSetStatus(A);
    }

    void EOR(uint8_t value)
    {
        A ^= value;

        LogicalSetStatus(A);
    }

    void ASL(uint8_t value)
    {
        A = (value << 1);

        LeftSetStatus(value, A);
    }

    void LSR(uint8_t value)
    {
        A = (value >> 1);

        LeftSetStatus(value, A);
    }

    void ROL(uint8_t value)
    {
        int8_t lastBit = (value & 0x80) >> 7;

        A = (value << 1) | lastBit;

        RightSetStatus(value, A);
    }

    void ROR(uint8_t value)
    {
        uint8_t firstBit = value & 1;

        A = (value >> 1) | (firstBit << 7);

        RightSetStatus(value, A);
    }


    // TODO: make this function better, it doesn't cover all cases.
    uint8_t BCD(uint8_t x, uint8_t y)
    {
        uint8_t first = (0b00001111 & x) + (0b00001111 & y);
        uint8_t second = (0b11110000 & x) + (0b11110000 & y);

        if (first > 0x09) first += 6;
        
        if (second > 0x09) second += 6;

        printf("BCD addition: %04x\n", ((second >> 4) * 10) + first);

        return ((second >> 4) * 10) + first;
    }

    /* Branch Helpers */
    void Branch(uint32_t &cycles, Memory &memory)
    {
        int8_t offset = FetchByte(cycles, memory);
        
        uint16_t oldPC = PC;

        /* Branch taken (+1 cycle) */
        PC += offset;
        cycles--;

        /* Page crossing (+1 cycle) */
        if ((oldPC & 0xFF00) != (PC & 0xFF00)) cycles--;
    }

    /* Setting statuses */
    void LDSetStatus(uint8_t reg)
    {
        SetStatusBit(ZERO_BIT, reg == 0);

        SetStatusBit(NEGATIVE_BIT, (reg & 0b10000000) > 0);
    }

    void CMPSetStatus(uint8_t value, uint8_t reg)
    {
        SetStatusBit(CARRY_BIT, reg >= value);
        SetStatusBit(ZERO_BIT, reg == value);
        SetStatusBit(NEGATIVE_BIT, ((reg - value) & 0b10000000) > 0);
    }

    void LogicalSetStatus(uint8_t value)
    {
        SetStatusBit(ZERO_BIT, value == 0);

        /* Set if bit 7 of the result is set */
        SetStatusBit(NEGATIVE_BIT, (value & 0b10000000) >> 7);
    }

    /* Shift and rotate left statuses */
    void LeftSetStatus(uint8_t oldValue, uint8_t value)
    {
        /* Set to contents of old bit 7 */
        SetStatusBit(CARRY_BIT, oldValue & 0b10000000);

        SetStatusBit(ZERO_BIT, value == 0);

        /* Set if bit 7 of the result is set */
        SetStatusBit(NEGATIVE_BIT, (value & 0b10000000) >> 7);
    }

    /* Shift and rotate left statuses */
    void RightSetStatus(uint8_t oldValue, uint8_t value)
    {
        /* Set to contents of old bit 7 */
        SetStatusBit(CARRY_BIT, oldValue & 1);

        SetStatusBit(ZERO_BIT, value == 0);

        /* Set if bit 7 of the result is set */
        SetStatusBit(NEGATIVE_BIT, (value & 0b10000000) >> 7);
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

        /* RTI → Pulls the processor flags from the stack followed by the program counter. */
        INS_RTI = 0x40,

        /* ===== BREAK ===== */
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
        INS_ADC_IDY = 0x71,

        /* SBC → Subtracts the contents of a memory location to the accumulator together with the not of the carry bit. */
        INS_SBC_IM = 0xE9,
        INS_SBC_ZP = 0xE5,
        INS_SBC_ZPX = 0xF5,
        INS_SBC_ABS = 0xED,
        INS_SBC_ABSX = 0xFD,
        INS_SBC_ABSY = 0xF9,
        INS_SBC_IDX = 0xE1,
        INS_SBC_IDY = 0xF1,

        /* ===== PSR OPERATIONS ===== */
        /* CLC → Clears the carry flag. */
        INS_CLC = 0x18,

        /* CLC → Sets the carry flag. */
        INS_SEC = 0x38,

        /* CLV → Clears the overflow flag. */
        INS_CLV = 0xB8,

        /* SED → Sets the decimal mode flag. */
        INS_SED = 0xF8,

        /* CLD → Clears the decimal mode flag. */
        INS_CLD = 0xD8,

        /* CLI → Clears the interrupt disable flag. */
        INS_CLI = 0x58,

        /* SEI → Sets the interrupt disable flag. */
        INS_SEI = 0x78,

        /* ===== LOGICAL OPERATIONS ===== */
        /* AND */
        INS_AND_IM = 0x29,
        INS_AND_ZP = 0x25,
        INS_AND_ZPX = 0x35,
        INS_AND_ABS = 0x2D,
        INS_AND_ABSX = 0x3D,
        INS_AND_ABSY = 0x39,
        INS_AND_IDX = 0x21,
        INS_AND_IDY = 0x31,

        /* ORA -> OR */
        INS_ORA_IM = 0x09,
        INS_ORA_ZP = 0x05,
        INS_ORA_ZPX = 0x15,
        INS_ORA_ABS = 0x0D,
        INS_ORA_ABSX = 0x1D,
        INS_ORA_ABSY = 0x19,
        INS_ORA_IDX = 0x01,
        INS_ORA_IDY = 0x11,

        /* EOR -> XOR */
        INS_EOR_IM = 0x49,
        INS_EOR_ZP = 0x45,
        INS_EOR_ZPX = 0x55,
        INS_EOR_ABS = 0x4D,
        INS_EOR_ABSX = 0x5D,
        INS_EOR_ABSY = 0x59,
        INS_EOR_IDX = 0x41,
        INS_EOR_IDY = 0x51,

        /* ===== ROTATE AND SHIFT OPERATIONS ===== */
        /* ASL -> Arithmetic Shift Left */
        INS_ASL_A = 0x0A,
        INS_ASL_ZP = 0x06,
        INS_ASL_ZPX = 0x16,
        INS_ASL_ABS = 0x0E,
        INS_ASL_ABSX = 0x1E,

        /* LSR -> Logical Shift Right */
        INS_LSR_A = 0x4A,
        INS_LSR_ZP = 0x46,
        INS_LSR_ZPX = 0x56,
        INS_LSR_ABS = 0x4E,
        INS_LSR_ABSX = 0x5E,

        /* ROL -> Arithmetic Shift Left */
        INS_ROL_A = 0x2A,
        INS_ROL_ZP = 0x26,
        INS_ROL_ZPX = 0x36,
        INS_ROL_ABS = 0x2E,
        INS_ROL_ABSX = 0x3E,

        /* ROR -> Arithmetic Shift Left */
        INS_ROR_A = 0x6A,
        INS_ROR_ZP = 0x66,
        INS_ROR_ZPX = 0x76,
        INS_ROR_ABS = 0x6E,
        INS_ROR_ABSX = 0x7E,


        /* ===== STACK OPERATIONS ===== */
        /* TXS -> Copies the current contents of the X register into the stack register. */
        INS_TXS = 0x9A,

        /* TSX -> Copies the current contents of the stack register into the X register, and sets Z and N flags. */
        INS_TSX = 0xBA,

        /* PHA -> Pushes a copy of the accumulator on to the stack. */
        INS_PHA = 0x48,

        /* PLA -> Pulls an 8 bit value from the stack and into the accumulator. */
        INS_PLA = 0x68,
        
        /* PHP -> Pushes a copy of the status flags on to the stack. */
        INS_PHP = 0x08,

        /* PLP -> Pulls an 8 bit value from the stack and into the processor flags. */
        INS_PLP = 0x28,


        /* ===== Bit Test ===== */
        INS_BIT_ZP = 0x24,
        INS_BIT_ABS = 0x2C;

    void Debug() {
        printf("A: %02X\nX: %02X\nY: %02X\nPC: %04X\nS: %04X\n", A, X, Y, PC, S);
        cout << "P: " << bitset<8>(P) << endl;
    }
};


