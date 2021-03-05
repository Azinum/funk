// vm.h

#ifndef _VM_H
#define _VM_H

#define MAX_STACK 512

typedef struct VM_state {
  i32* program;
  i32 program_size;
} VM_state;

i32 vm_init(struct VM_state* vm);

i32 vm_exec(struct VM_state* vm, char* file, char* source);

void vm_free(struct VM_state* vm);

#endif
