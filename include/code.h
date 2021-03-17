// code.h

#ifndef _CODE_H
#define _CODE_H

enum Instruction {
  I_EXIT = 0,
  I_UNKNOWN,
  I_NOP,

  I_PUSH,
  I_PUSH_ARG,
  I_POP,
  I_ASSIGN,
  I_COND_JUMP,
  I_JUMP,
  I_RETURN,
  I_CALL,

  I_ADD,
  I_SUB,
  I_MUL,
  I_DIV,

  I_LT,
  I_GT,
  I_EQ,

  MAX_INS,
};

struct VM_state;

i32 code_gen(struct VM_state* vm, Ast* ast);

#endif
