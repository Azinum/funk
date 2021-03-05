// code.h

#ifndef _CODE_H
#define _CODE_H

enum Instruction {
  I_EXIT = 0,
  I_UNKNOWN,
  I_NOP,

  I_PUSH,
  I_POP,

  I_ADD,
  I_SUB,
  I_MUL,
  I_DIV,
};

struct VM_state;

i32 code_gen(struct VM_state* vm, Ast* ast);

#endif
