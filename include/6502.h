// 6502.h

#ifndef _6502_H
#define _6502_H

#include "hash.h"

struct Compile_state {
  i32 status;
  i8* program;
  i32 program_size;
  i32 data_section;
  Htable symbol_table;
};

i32 run_6502(char* path);

#endif
