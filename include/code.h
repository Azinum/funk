// code.h

#ifndef _CODE_H
#define _CODE_H

enum Instruction {
  I_NOP = 0,
};

i32 code_gen(Ast* ast, i32** program, i32* size);

#endif
