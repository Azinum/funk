// vm.c
// Virtual machine (executes byte code)

#include "common.h"
#include "ast.h"
#include "parser.h"
#include "code.h"
#include "vm.h"

#define runtime_error(fmt, ...) \
  fprintf(stderr, "runtime-error: " fmt, ##__VA_ARGS__)

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
inline i32 object_check_true(struct Object* obj);
static i32 execute(struct VM_state* vm);
static void stack_print_all(struct VM_state* vm);

i32 vm_init(struct VM_state* vm) {
  vm->stack_top = 0;
  vm->values = NULL;
  vm->values_count = 0;
  func_init(&vm->global, NULL);
  func_state_init(&vm->fs_global, NULL, &vm->global);
  vm->program = NULL;
  vm->program_size = 0;
  vm->old_program_size = 0;
  vm->ip = NULL;
  vm->saved_ip = 0;
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

i32 object_check_true(struct Object* obj) {
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
    i32 ins = *(vm->ip++);
    switch (ins) {
      case I_EXIT:
        return NO_ERR;
      case I_NOP:
        break;

      case I_PUSH: {
        i32 address = *(vm->ip++);
        assert(address >= 0 && address < vm->values_count);
        struct Object obj = vm->values[address];
        stack_push(vm, obj);
#if 0
        printf("I_PUSH: %i (value = ", address);
        object_print(stdout, &obj);
        printf(")\n");
#endif
        break;
      }
      case I_POP:
        break;
      case I_ASSIGN: {
        i32 address = *(vm->ip++);
        struct Object* left = &vm->values[address];
        const struct Object* right = stack_get_top(vm);
        if (!right) {
          assert(0);
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

        if (!object_check_true(obj)) {
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
      case I_CALL: {
        i32 address = *(vm->ip++);
        assert(address >= 0 && address < vm->program_size);
        i32* old_ip = vm->ip; // Save the current instruction pointer position
        vm->ip = &vm->program[address];
        execute(vm);  // Execute function
        vm->ip = old_ip; // Restore the old instruction pointer
        break;
      }
      case I_RETURN: {
        return NO_ERR;
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
        runtime_error("Tried to execute bad instruction (%i)\n", ins);
        assert(0);
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
#if 1
    if (code_gen(vm, &ast) == NO_ERR) {
#if 1
      if (vm->program_size > 0) {
        if (!vm->ip) {
          vm->ip = &vm->program[0];
        }
        if (vm->old_program_size != vm->program_size) {
          vm->ip = &vm->program[vm->saved_ip];
          execute(vm);
          stack_print_all(vm);
          list_shrink(vm->program, vm->program_size, 1); // Remove I_RETURN instruction
          vm->old_program_size = vm->program_size;
          // NOTE(lucas): The instruction pointer probably got invalidated
          // because the program was moved to another location (this can happen when generating new code/modifying the program list with list_shrink e.t.c.)
          vm->saved_ip = (i32)(&vm->program[vm->program_size] - &vm->program[0]); // Save the instruction pointer index, and restore it in the next execution.
          vm->stack_top = 0;
        }
      }
#endif
    }
    else {
      vm->status = NO_ERR;
    }
#endif
  }
  ast_free(&ast);
  return NO_ERR;
}

void vm_free(struct VM_state* vm) {
  for (i32 i = 0; i < vm->values_count; i++) {
    struct Object* obj = &vm->values[i];
    switch (obj->type) {
      case T_FUNCTION:
        func_free(&obj->value.func);
        break;
      default:
        break;
    }
  }
  list_free(vm->values, vm->values_count);
  func_free(&vm->global);
  func_state_free(&vm->fs_global);
  list_free(vm->program, vm->program_size);
  vm->ip = NULL;
}
