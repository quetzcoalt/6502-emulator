# 6502 Emulator

It's an 8-bit CPU capable of handling at most 64 Kb of memory via its 16-bit address bus.

## Registers
It has few registers:
- A = 8-bit Accumulator.
- P = 8-bit processor status with 7 flags:
    - n = negative;
    - v = overflow;
    - b = break;
    - d = decimal;
    - i = interrupt disable;
    - z = zero;
    - c = carry.
- PC = 16-bit Program Counter.
- S = 8-bit Stack Pointer.
- X = 8-bit index register;
- Y = 8-bit index register;

## Memory Addressing
The CPU can address a maximum of 64 KB of  addresses due to its 16-bit address bus:
- 2^16 = 65536 = 64 * 1024 = 64 KB

### Zero page
- The first 256 bytes of memory are referred to as "Zero Page".
- From `0x0000` to `0x00FF`.

> The size of the Zero Page depends on the size of the index register of the processor. The 6502 index registers' are 8-bit, which implies a 2^8 = 256 bytes.

- Only the least significant byte of the address is held in the instruction, resulting in:
    - shorter memory address by one byte (important for space saving). 
    - one less memory fetch during execution (important for speed).

### Stack
- Hardcoded from address `0x0100` to address `0x01FF`.

## Resources I use
- [6502 - Obelisk by Andrew Jacobs](https://6502.org/users/obelisk/6502): The reference is very well made and concise with a table of instructions and how many bytes and cycles each instruction consumes;
- 6502 Machine Code For Beginners - AP Stephenson (🕮);
- Wikipedia 6502 article;
- [6502 CPU Emulator in C++ playlist](https://www.youtube.com/playlist?list=PLLwK93hM93Z13TRzPx9JqTIn33feefl37): the first video game me a good start and motivated me to keep on writing code for each instruction. 

## TO-DOs later
- [ ] LDA, LDX and LDY share common implementations → Implementing a common function for each instruction depending on the addressing mode.
- [ ] Taking care of overflows and underflows.