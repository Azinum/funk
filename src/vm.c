// vm.c
// Virtual machine (executes byte code)

#include "common.h"
#include "ast.h"
#include "parser.h"
#include "code.h"
#include "vm.h"

static i32 execute(struct VM_state* vm);

i32 vm_init(struct VM_state* vm) {
  vm->program = NULL;
  vm->program_size = 0;
  return NO_ERR;
}

// TODO(lucas): Implement jump table
i32 execute(struct VM_state* vm) {
  i32* ip = &vm->program[0];
  (void)ip;
  i32 ins = I_NOP;
  for (;;) {
    switch (ins) {
      case I_NOP:
        return NO_ERR;
      default:
        return NO_ERR;
    }
  }
  return NO_ERR;
}

i32 vm_exec(struct VM_state* vm, char* file, char* source) {
  Ast ast = ast_create();
  if (parser_parse(source, file, &ast) == NO_ERR) {
    ast_print(ast);
    if (code_gen(&ast, &vm->program, &vm->program_size) == NO_ERR) {
      if (vm->program_size > 0) {
        execute(vm);
      }
      printf("Ok\n");
    }
  }
  ast_free(&ast);
  return NO_ERR;
}

void vm_free(struct VM_state* vm) {
  (void)vm;
  // TODO(lucas): Implement
}
