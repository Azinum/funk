// vm.h

#ifndef _VM_H
#define _VM_H

#include "object.h"
#include "hash.h"
#include "list.h"
#include "buffer.h"

#define MAX_STACK 512

typedef struct VM_state {
  struct Object stack[MAX_STACK];
  i32 stack_top;
  i32 stack_base;
  struct Object* values;
  i32 values_count;
  struct Buffer buffer;
  struct Function global;
  struct Function_state fs_global;
  i32* program;
  i32 program_size;
  i32 old_program_size;
  i32* ip;
  i32 saved_ip;
  i32 status;
} VM_state;

i32 vm_init(struct VM_state* vm);

i32 vm_exec(struct VM_state* vm, char* file, char* source);

void vm_free(struct VM_state* vm);

#endif
