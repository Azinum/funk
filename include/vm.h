// vm.h

#ifndef _VM_H
#define _VM_H

#include "object.h"
#include "hash.h"
#include "list.h"

#define MAX_STACK 512

typedef struct VM_state {
  struct Object stack[MAX_STACK];
  i32 stack_top;
  struct Object* constants;
  i32 constant_count;
  Htable symbol_table;
  i32* program;
  i32 program_size;
  i32* ip;
  i32 status;
} VM_state;

i32 vm_init(struct VM_state* vm);

i32 vm_exec(struct VM_state* vm, char* file, char* source);

void vm_free(struct VM_state* vm);

#endif
