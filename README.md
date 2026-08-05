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

### Instructions implemented
I make sure to implement the instructions in all their possible memory addressing modes before moving to the next one; 
- Currenlty, I have implemented <span style="color:khaki; font-weight:500">20</span> out if 56 instruction.

<table border="1" cellpadding="0" cellspacing="0" width="450">
  <tbody>
    <tr>
      <td width="7%" height="25"><p><input type="checkbox"> ADC</p></td>
      <td width="7%"><p><input type="checkbox"> AND</p></td>
      <td width="7%"><p><input type="checkbox"> ASL</p></td>
      <td width="7%"><p><input type="checkbox" checked> BCC</p></td>
      <td width="7%"><p><input type="checkbox" checked> BCS</p></td>
      <td width="7%"><p><input type="checkbox" checked> BEQ</p></td>
      <td width="7%"><p><input type="checkbox"> BIT</p></td>
      <td width="7%"><p><input type="checkbox" checked> BMI</p></td>
      <td width="7%"><p><input type="checkbox" checked> BNE</p></td>
      <td width="7%"><p><input type="checkbox" checked> BPL</p></td>
      <td width="7%"><p><input type="checkbox"> BRK</p></td>
      <td width="7%"><p><input type="checkbox" checked> BVC</p></td>
      <td width="7%"><p><input type="checkbox" checked> BVS</p></td>
      <td width="7%"><p><input type="checkbox"> CLC</p></td>
    </tr>
    <tr>
      <td width="7%" height="25"><p><input type="checkbox"> CLD</p></td>
      <td width="7%"><p><input type="checkbox"> CLI</p></td>
      <td width="7%"><p><input type="checkbox"> CLV</p></td>
      <td width="7%"><p><input type="checkbox" checked> CMP</p></td>
      <td width="7%"><p><input type="checkbox" checked> CPX</p></td>
      <td width="7%"><p><input type="checkbox" checked> CPY</p></td>
      <td width="7%"><p><input type="checkbox"> DEC</p></td>
      <td width="7%"><p><input type="checkbox"> DEX</p></td>
      <td width="7%"><p><input type="checkbox"> DEY</p></td>
      <td width="7%"><p><input type="checkbox"> EOR</p></td>
      <td width="7%"><p><input type="checkbox" checked> INC</p></td>
      <td width="7%"><p><input type="checkbox" checked> INX</p></td>
      <td width="7%"><p><input type="checkbox" checked> INY</p></td>
      <td width="7%"><p><input type="checkbox"> JMP</p></td>
    </tr>
    <tr>
      <td width="7%" height="25"><p><input type="checkbox"> JSR</p></td>
      <td width="7%"><p><input type="checkbox" checked> LDA</p></td>
      <td width="7%"><p><input type="checkbox" checked> LDX</p></td>
      <td width="7%"><p><input type="checkbox" checked> LDY</p></td>
      <td width="7%"><p><input type="checkbox"> LSR</p></td>
      <td width="7%"><p><input type="checkbox"> NOP</p></td>
      <td width="7%"><p><input type="checkbox"> ORA</p></td>
      <td width="7%"><p><input type="checkbox"> PHA</p></td>
      <td width="7%"><p><input type="checkbox"> PHP</p></td>
      <td width="7%"><p><input type="checkbox"> PLA</p></td>
      <td width="7%"><p><input type="checkbox"> PLP</p></td>
      <td width="7%"><p><input type="checkbox"> ROL</p></td>
      <td width="7%"><p><input type="checkbox"> ROR</p></td>
      <td width="7%"><p><input type="checkbox"> RTI</p></td>
    </tr>
    <tr>
      <td width="7%" height="25"><p><input type="checkbox"> RTS</p></td>
      <td width="7%"><p><input type="checkbox"> SBC</p></td>
      <td width="7%"><p><input type="checkbox"> SEC</p></td>
      <td width="7%"><p><input type="checkbox"> SED</p></td>
      <td width="7%"><p><input type="checkbox"> SEI</p></td>
      <td width="7%"><p><input type="checkbox" checked> STA</p></td>
      <td width="7%"><p><input type="checkbox" checked> STX</p></td>
      <td width="7%"><p><input type="checkbox" checked> STY</p></td>
      <td width="7%"><p><input type="checkbox"> TAX</p></td>
      <td width="7%"><p><input type="checkbox"> TAY</p></td>
      <td width="7%"><p><input type="checkbox"> TSX</p></td>
      <td width="7%"><p><input type="checkbox"> TXA</p></td>
      <td width="7%"><p><input type="checkbox"> TXS</p></td>
      <td width="7%"><p><input type="checkbox"> TYA</p></td>
    </tr>
  </tbody>
</table>

## Todo later
- [ ] LDA, LDX and LDY share common implementations → Implementing a common function for each instruction depending on the addressing mode.
- [ ] Taking care of overflows and underflows.