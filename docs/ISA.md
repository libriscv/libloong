# LoongArch ISA Support

## Implemented Instructions

libloong currently implements a subset of the LoongArch instruction set, focusing on the most commonly used instructions for Linux user-space applications.

### Status Overview

- ✅ Basic integer arithmetic
- ✅ Load/Store operations
- ✅ Branch instructions
- ✅ System calls
- ⚠️ Partial floating-point support
- ⚠️ Partial atomic operations
- ❌ Vector extensions (to be implemented)
- ❌ Privileged instructions (not needed for user-space)

## Integer Operations

### Arithmetic

**LA32 Instructions:**
- `ADD.W rd, rj, rk` - Add word
- `SUB.W rd, rj, rk` - Subtract word
- `ADDI.W rd, rj, imm12` - Add immediate word

**LA64 Instructions:**
- `ADD.D rd, rj, rk` - Add doubleword
- `SUB.D rd, rj, rk` - Subtract doubleword
- `ADDI.D rd, rj, imm12` - Add immediate doubleword

### Logical Operations

- `AND rd, rj, rk` - Bitwise AND
- `OR rd, rj, rk` - Bitwise OR
- `XOR rd, rj, rk` - Bitwise XOR
- `NOR rd, rj, rk` - Bitwise NOR
- `ANDI rd, rj, imm12` - AND immediate
- `ORI rd, rj, imm12` - OR immediate
- `XORI rd, rj, imm12` - XOR immediate

### Shift Operations

**Word shifts:**
- `SLL.W rd, rj, rk` - Shift left logical word
- `SRL.W rd, rj, rk` - Shift right logical word
- `SRA.W rd, rj, rk` - Shift right arithmetic word

**Doubleword shifts (LA64 only):**
- `SLL.D rd, rj, rk` - Shift left logical doubleword
- `SRL.D rd, rj, rk` - Shift right logical doubleword
- `SRA.D rd, rj, rk` - Shift right arithmetic doubleword

## Load/Store Instructions

### Load Operations

**Signed loads:**
- `LD.B rd, rj, imm12` - Load byte (sign-extended)
- `LD.H rd, rj, imm12` - Load halfword (sign-extended)
- `LD.W rd, rj, imm12` - Load word (sign-extended in LA64)
- `LD.D rd, rj, imm12` - Load doubleword (LA64 only)

**Unsigned loads:**
- `LD.BU rd, rj, imm12` - Load byte unsigned
- `LD.HU rd, rj, imm12` - Load halfword unsigned
- `LD.WU rd, rj, imm12` - Load word unsigned (LA64 only)

### Store Operations

- `ST.B rd, rj, imm12` - Store byte
- `ST.H rd, rj, imm12` - Store halfword
- `ST.W rd, rj, imm12` - Store word
- `ST.D rd, rj, imm12` - Store doubleword (LA64 only)

## Branch Instructions

### Conditional Branches

- `BEQ rj, rd, offs16` - Branch if equal
- `BNE rj, rd, offs16` - Branch if not equal
- `BLT rj, rd, offs16` - Branch if less than (signed)
- `BGE rj, rd, offs16` - Branch if greater or equal (signed)
- `BLTU rj, rd, offs16` - Branch if less than (unsigned)
- `BGEU rj, rd, offs16` - Branch if greater or equal (unsigned)

### Unconditional Jumps

- `B offs26` - Unconditional branch
- `BL offs26` - Branch and link
- `JIRL rd, rj, offs16` - Jump indirect and link

## Upper Immediate Instructions

- `LU12I.W rd, imm20` - Load upper 12 bits immediate (word)
- `LU32I.D rd, imm20` - Load upper 32 bits immediate (LA64 only)
- `PCADDU12I rd, imm20` - PC-relative add upper immediate

## Multiply/Divide

**Word operations:**
- `MUL.W rd, rj, rk` - Multiply word
- `MULH.W rd, rj, rk` - Multiply high word (signed)
- `MULH.WU rd, rj, rk` - Multiply high word (unsigned)
- `DIV.W rd, rj, rk` - Divide word (signed)
- `MOD.W rd, rj, rk` - Modulo word (signed)
- `DIV.WU rd, rj, rk` - Divide word (unsigned)
- `MOD.WU rd, rj, rk` - Modulo word (unsigned)

**Doubleword operations (LA64 only):**
- `MUL.D rd, rj, rk` - Multiply doubleword
- `MULH.D rd, rj, rk` - Multiply high doubleword (signed)
- `MULH.DU rd, rj, rk` - Multiply high doubleword (unsigned)
- `DIV.D rd, rj, rk` - Divide doubleword (signed)
- `MOD.D rd, rj, rk` - Modulo doubleword (signed)
- `DIV.DU rd, rj, rk` - Divide doubleword (unsigned)
- `MOD.DU rd, rj, rk` - Modulo doubleword (unsigned)

## System Instructions

- `SYSCALL code15` - System call
- `BREAK code15` - Breakpoint

## Register Conventions

### General Purpose Registers (GPRs)

| Register | ABI Name | Purpose | Saved Across Calls |
|----------|----------|---------|-------------------|
| $r0      | $zero    | Always zero | N/A |
| $r1      | $ra      | Return address | No |
| $r2      | $tp      | Thread pointer | - |
| $r3      | $sp      | Stack pointer | Yes |
| $r4-$r11 | $a0-$a7  | Arguments/return values | No |
| $r12-$r20| $t0-$t8  | Temporaries | No |
| $r21     | -        | Reserved | - |
| $r22     | $fp/$s9  | Frame pointer | Yes |
| $r23-$r31| $s0-$s8  | Saved registers | Yes |

### Floating-Point Registers

- $f0-$f7: Argument/return registers
- $f8-$f23: Temporary registers
- $f24-$f31: Saved registers

## System Call Interface

System calls use the following register convention:
- $a7 ($r11): System call number
- $a0-$a5 ($r4-$r9): Arguments
- $a0 ($r4): Return value

## Notes

1. **Register $r0** is hardwired to zero - writes are ignored
2. **Instruction alignment**: All instructions are 4 bytes and must be 4-byte aligned
3. **Memory model**: Little-endian byte order
4. **Address widths**:
   - LA32: 32-bit addresses (4 GB address space)
   - LA64: 64-bit addresses (full 64-bit address space)

## Planned Extensions

Future versions will add support for:
- Complete floating-point instruction set
- Atomic memory operations
- LoongArch vector extensions (LSX/LASX)
- Performance counters
- Additional privileged instructions (for future system emulation)

## Instruction Format Reference

### Instruction Types

1. **2R-type**: `op rd, rj`
2. **3R-type**: `op rd, rj, rk`
3. **4R-type**: `op rd, rj, rk, ra`
4. **2RI8-type**: `op rd, rj, imm8`
5. **2RI12-type**: `op rd, rj, imm12`
6. **2RI14-type**: `op rd, rj, imm14`
7. **2RI16-type**: `op rj, rd, offs16`
8. **1RI20-type**: `op rd, imm20`
9. **1RI21-type**: `op rj, offs21`
10. **I26-type**: `op offs26`

All instructions are 32 bits wide with fixed encoding.
