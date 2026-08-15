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
Icons preceding each resource indicate the type of resource.
- 🌐︎ [6502 - Obelisk by Andrew Jacobs](https://6502.org/users/obelisk/6502): The reference is very well made and concise with a table of instructions and how many bytes and cycles each instruction consumes;
- 🕮 6502 Machine Code For Beginners - AP Stephenson;
- Wikipedia 6502 article;
- ▷ [6502 CPU Emulator in C++ playlist](https://www.youtube.com/playlist?list=PLLwK93hM93Z13TRzPx9JqTIn33feefl37) - the first video game me a good start and motivated me to keep on writing code for each instruction.
- 🌐︎ [6502 | Ultimate Commodore Reference](https://www.pagetable.com/c64ref/6502/) - Great details about the addressing modes and instructions.
- 🌐︎ [Overflow Flag Explained - Ken Sherrif's blog](https://www.righto.com/2012/12/the-6502-overflow-flag-explained.html).

### Instructions implemented
I make sure to implement the instructions in all their possible memory addressing modes before moving to the next one; 
- Currenlty, I have implemented <span style="color:khaki; font-weight:500">34</span> out if 56 instruction.

<table border="1" cellpadding="0" cellspacing="0" width="450">
  <tbody>
    <tr>
      <td width="7%" height="25"><p>☑ ADC</p></td>
      <td width="7%"><p>☑ AND</p></td>
      <td width="7%"><p>☐ ASL</p></td>
      <td width="7%"><p>☑ BCC</p></td>
      <td width="7%"><p>☑ BCS</p></td>
      <td width="7%"><p>☑ BEQ</p></td>
      <td width="7%"><p>☐ BIT</p></td>
      <td width="7%"><p>☑ BMI</p></td>
      <td width="7%"><p>☑ BNE</p></td>
      <td width="7%"><p>☑ BPL</p></td>
      <td width="7%"><p>☐ BRK</p></td>
      <td width="7%"><p>☑ BVC</p></td>
      <td width="7%"><p>☑ BVS</p></td>
      <td width="7%"><p>☑ CLC</p></td>
    </tr>
    <tr>
      <td width="7%" height="25"><p>☑ CLD</p></td>
      <td width="7%"><p>☐ CLI</p></td>
      <td width="7%"><p>☑ CLV</p></td>
      <td width="7%"><p>☑ CMP</p></td>
      <td width="7%"><p>☑ CPX</p></td>
      <td width="7%"><p>☑ CPY</p></td>
      <td width="7%"><p>☐ DEC</p></td>
      <td width="7%"><p>☐ DEX</p></td>
      <td width="7%"><p>☐ DEY</p></td>
      <td width="7%"><p>☑ EOR</p></td>
      <td width="7%"><p>☑ INC</p></td>
      <td width="7%"><p>☑ INX</p></td>
      <td width="7%"><p>☑ INY</p></td>
      <td width="7%"><p>☑ JMP</p></td>
    </tr>
    <tr>
      <td width="7%" height="25"><p>☑ JSR</p></td>
      <td width="7%"><p>☑ LDA</p></td>
      <td width="7%"><p>☑ LDX</p></td>
      <td width="7%"><p>☑ LDY</p></td>
      <td width="7%"><p>☐ LSR</p></td>
      <td width="7%"><p>☑ NOP</p></td>
      <td width="7%"><p>☑ ORA</p></td>
      <td width="7%"><p>☐ PHA</p></td>
      <td width="7%"><p>☐ PHP</p></td>
      <td width="7%"><p>☐ PLA</p></td>
      <td width="7%"><p>☐ PLP</p></td>
      <td width="7%"><p>☐ ROL</p></td>
      <td width="7%"><p>☐ ROR</p></td>
      <td width="7%"><p>☐ RTI</p></td>
    </tr>
    <tr>
      <td width="7%" height="25"><p>☑ RTS</p></td>
      <td width="7%"><p>☑ SBC</p></td>
      <td width="7%"><p>☑ SEC</p></td>
      <td width="7%"><p>☑ SED</p></td>
      <td width="7%"><p>☐ SEI</p></td>
      <td width="7%"><p>☑ STA</p></td>
      <td width="7%"><p>☑ STX</p></td>
      <td width="7%"><p>☑ STY</p></td>
      <td width="7%"><p>☐ TAX</p></td>
      <td width="7%"><p>☐ TAY</p></td>
      <td width="7%"><p>☐ TSX</p></td>
      <td width="7%"><p>☐ TXA</p></td>
      <td width="7%"><p>☐ TXS</p></td>
      <td width="7%"><p>☐ TYA</p></td>
    </tr>
  </tbody>
</table>

## Todo later
- [ ] LDA, LDX and LDY share common implementations → Implementing a common function for each instruction depending on the addressing mode.
- [ ] Taking care of overflows and underflows.

## To run tests
First:
```bash
cmake -S . -B build
```

And then, just keep doing:
```bash
cmake --build build
cd build && ctest
```