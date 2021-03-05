// vm.c
// Virtual machine (executes byte code)

#include "common.h"
#include "ast.h"
#include "parser.h"
#include "code.h"
#include "vm.h"

#define runtime_error(fmt, ...) \
  fprintf(stderr, "runtime-error: " fmt ##__VA_ARGS__)

#define ARITH(VM, OP) { \
  if (VM->stack_top >= 2) { \
    struct Object* left = &VM->stack[VM->stack_top - 2]; \
    const struct Object* right = &VM->stack[VM->stack_top - 1]; \
    left->value.number = left->value.number OP right->value.number; \
    stack_pop(VM); \
  } \
  else { \
    runtime_error("Not enough arguments for arithmetic operation\n"); \
    VM->status = ERR; \
    goto done; \
  }\
} \

inline void stack_push(struct VM_state* vm, struct Object obj);
inline void stack_pop(struct VM_state* vm);
static i32 execute(struct VM_state* vm);

i32 vm_init(struct VM_state* vm) {
  vm->stack_top = 0;
  vm->constants = NULL;
  vm->constant_count = 0;
  vm->symbol_table = ht_create_empty();
  vm->program = NULL;
  vm->program_size = 0;
  vm->ip = NULL;
  vm->status = NO_ERR;
  return NO_ERR;
}

void stack_push(struct VM_state* vm, struct Object obj) {
  if (vm->stack_top < MAX_STACK) {
    vm->stack[vm->stack_top++] = obj;
  }
}

void stack_pop(struct VM_state* vm) {
  if (vm->stack_top > 0) {
    --vm->stack_top;
  }
}

i32 execute(struct VM_state* vm) {
  i32* ip = vm->ip;
  for (;;) {
    switch (*(ip++)) {
      case I_EXIT:
        return NO_ERR;
      case I_NOP:
        break;

      case I_PUSH: {
        i32 address = *(ip++);
        assert(address >= 0 && address < vm->constant_count);
        const struct Object obj = vm->constants[address];
        stack_push(vm, obj);
        break;
      }
      case I_POP:
        break;
      case I_ADD:
        ARITH(vm, +);
        break;
      case I_SUB:
        ARITH(vm, -);
        break;
      case I_MUL:
        ARITH(vm, *);
        break;
      case I_DIV:
        ARITH(vm, /);
        break;
      default:
        return vm->status;
    }
  }
done:
  return vm->status;
}

i32 vm_exec(struct VM_state* vm, char* file, char* source) {
  Ast ast = ast_create();
  if (parser_parse(source, file, &ast) == NO_ERR) {
    if (code_gen(vm, &ast) == NO_ERR) {
      if (vm->program_size > 0) {
        if (!vm->ip) {
          vm->ip = &vm->program[0];
        }
        execute(vm);
        // ast_print(ast);
        printf("program_size: %i, constant_count: %i, stack_top: %i\n", vm->program_size, vm->constant_count, vm->stack_top);
        if (vm->stack_top > 0) {
          struct Object top = vm->stack[vm->stack_top - 1];
          object_printline(stdout, &top);
        }
      }
    }
  }
  ast_free(&ast);
  return NO_ERR;
}

void vm_free(struct VM_state* vm) {
  list_free(vm->constants, vm->constant_count);
  ht_free(&vm->symbol_table);
  list_free(vm->program, vm->program_size);
  vm->ip = NULL;
}
