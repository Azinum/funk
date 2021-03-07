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
    struct Object* left = stack_get(VM, 1); \
    const struct Object* right = stack_get(VM, 0); \
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
inline struct Object* stack_get(struct VM_state* vm, i32 offset);
inline struct Object* stack_get_top(struct VM_state* vm);
inline i32 object_check_truth(struct VM_state* vm, struct Object* obj);
static i32 execute(struct VM_state* vm);
static void stack_print_all(struct VM_state* vm);

i32 vm_init(struct VM_state* vm) {
  vm->stack_top = 0;
  vm->constants = NULL;
  vm->constant_count = 0;
  vm->symbol_table = ht_create_empty();
  vm->program = NULL;
  vm->program_size = 0;
  vm->ip = NULL;
  vm->status = NO_ERR;
  vm->old_program_size = 0;
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

struct Object* stack_get(struct VM_state* vm, i32 offset) {
  i32 index = vm->stack_top - (offset + 1);
  if (index >= 0) {
    return &vm->stack[index];
  }
  return NULL;
}

struct Object* stack_get_top(struct VM_state* vm) {
  if (vm->stack_top > 0) {
    return &vm->stack[vm->stack_top - 1];
  }
  return NULL;
}

i32 object_check_truth(struct VM_state* vm, struct Object* obj) {
  switch (obj->type) {
    case T_NUMBER:
      return obj->value.number != 0;
    default:
      break;
  }
  return 0;
}

i32 execute(struct VM_state* vm) {
  for (;;) {
    switch (*(vm->ip++)) {
      case I_EXIT:
        return NO_ERR;
      case I_NOP:
        break;

      case I_PUSH: {
        i32 address = *(vm->ip++);
        assert(address >= 0 && address < vm->constant_count);
        const struct Object obj = vm->constants[address];
        stack_push(vm, obj);
        break;
      }
      case I_POP:
        break;
      case I_ASSIGN: {
        i32 address = *(vm->ip++);
        struct Object* left = &vm->constants[address];
        const struct Object* right = stack_get_top(vm);
        if (!right) {
          // Handle error
          return ERR;
        }
        *left = *right;
        stack_pop(vm);
        break;
      }
      case I_COND_JUMP: {
        i32 offset = *(vm->ip++);
        struct Object* obj = stack_get_top(vm);
        assert(obj);

        if (!object_check_truth(vm, obj)) {
          stack_pop(vm);
          vm->ip += offset;
        }
        stack_pop(vm);
        break;
      }
      case I_JUMP: {
        i32 offset = *(vm->ip++);
        vm->ip += offset;
        break;
      }
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

void stack_print_all(struct VM_state* vm) {
  printf("[");
  for (i32 i = 0; i < vm->stack_top; i++) {
    struct Object* obj = &vm->stack[i];
    object_print(stdout, obj);
    if (i < vm->stack_top - 1) {
      printf(", ");
    }
  }
  printf("]\n");
}

i32 vm_exec(struct VM_state* vm, char* file, char* source) {
  Ast ast = ast_create();
  if (parser_parse(source, file, &ast) == NO_ERR) {
    // ast_print(ast);
    if (code_gen(vm, &ast) == NO_ERR) {
#if 1
      if (vm->program_size > 0) {
        if (!vm->ip) {
          vm->ip = &vm->program[0];
        }
        if (vm->old_program_size != vm->program_size) {
          execute(vm);
          stack_print_all(vm);
          list_shrink(vm->program, vm->program_size, 1); // Remove I_EXIT instruction
          vm->old_program_size = vm->program_size;
          vm->ip--;
          vm->stack_top = 0;
        }
      }
#endif
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
