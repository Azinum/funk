// 6502_code.h

#ifndef _6502_CODE_H
#define _6502_CODE_H

// http://6502.org/tutorials/6502opcodes.html
enum Opcode {
  OP_ADC_IMM = 0x69,

  OP_STY_ZPG = 0x84,
  OP_STA_ZPG = 0x85,
  OP_STX_ZPG = 0x86,

  OP_LDX_IMM = 0xa2,

  OP_LDA_IMM = 0xa9,
  OP_LDA_ZPG = 0xa5,
  OP_LDA_ZPG_X = 0xb5,

  OP_NOP = 0xea,
};

struct Compile_state;

i32 code_gen_6502(struct Compile_state* state, Ast* ast);

#endif
