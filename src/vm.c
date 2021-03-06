// vm.c
// Virtual machine (executes byte code)

#include "common.h"
#include "ast.h"
#include "parser.h"
#include "code.h"
#include "vm.h"

#define runtime_error(fmt, ...) \
  fprintf(stderr, "runtime-error: " fmt, ##__VA_ARGS__)

#define EQUAL_TYPES(a, b, t) ((a)->type == t && (b)->type == t)

#define ARITH(VM, OP) { \
  if (VM->stack_top >= 2) { \
    struct Object* left = stack_get(VM, 1); \
    const struct Object* right = stack_get(VM, 0); \
    if (EQUAL_TYPES(left, right, T_NUMBER)) { \
      left->value.number = left->value.number OP right->value.number; \
      stack_pop(VM); \
    } \
    else { \
      runtime_error("Invalid types in arithmetic operation\n"); \
      VM->status = ERR; \
      goto done; \
    } \
  } \
  else { \
    runtime_error("Not enough arguments for arithmetic operation\n"); \
    VM->status = ERR; \
    goto done; \
  }\
} \

inline void stack_push(struct VM_state* vm, struct Object obj);
inline struct Object* stack_pop(struct VM_state* vm);
inline struct Object* stack_get(struct VM_state* vm, i32 offset);
inline struct Object* stack_get_top(struct VM_state* vm);
inline i32 object_check_true(struct Object* obj);
inline i32 objects_are_equal(struct Object* a, struct Object* b);
static i32 vm_define_value(struct VM_state* vm, const char* name, struct Object value);
static i32 vm_define_function(struct VM_state* vm, const char* name, cfunction func, i32 argc);
static i32 vm_debug_print(struct VM_state* vm);
static i32 execute(struct VM_state* vm);
static void stack_print_all(struct VM_state* vm);

i32 vm_init(struct VM_state* vm) {
  vm->stack_top = 0;
  vm->stack_base = 0;
  vm->values = NULL;
  vm->values_count = 0;
  buffer_init(&vm->buffer);
  func_init(&vm->global, NULL);
  func_state_init(&vm->fs_global, NULL, &vm->global);
  vm->program = NULL;
  vm->program_size = 0;
  vm->old_program_size = 0;
  vm->ip = NULL;
  vm->saved_ip = 0;
  vm->status = NO_ERR;
  vm_define_function(vm, "print", vm_debug_print, 1);
  return NO_ERR;
}

void stack_push(struct VM_state* vm, struct Object obj) {
  if (vm->stack_top < MAX_STACK) {
    vm->stack[vm->stack_top++] = obj;
  }
  else {
    runtime_error("Stack overflow, reached stack limit of %i!\n", MAX_STACK);
    vm->status = ERR;
  }
}

struct Object* stack_pop(struct VM_state* vm) {
  if (vm->stack_top > 0) {
    return &vm->stack[--vm->stack_top];
  }
  return NULL;
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

i32 objects_are_equal(struct Object* a, struct Object* b) {
  if (a->type == b->type) {
    switch (a->type) {
      case T_NUMBER: {
        return a->value.number == b->value.number;
      }
      case T_FUNCTION: {
        return a->value.func.address == b->value.func.address;
      }
      case T_STRING: {
        if (a->value.buffer.data == b->value.buffer.data) {
          return 1;
        }
        else if (a->value.buffer.length == b->value.buffer.length) {
          return (strncmp(a->value.buffer.data, b->value.buffer.data, a->value.buffer.length)) == 0;
        }
      }
      default:
        break;
    }
  }
  return 0;
}

i32 vm_define_value(struct VM_state* vm, const char* name, struct Object value) {
  i32 address = -1;
  (void)address;
  const i32* found = ht_lookup(&vm->fs_global.symbol_table, name);
  if (found) {
    assert(0);
    return vm->status = ERR;
  }
  else {
    address = vm->values_count;
    list_push(vm->values, vm->values_count, value);
    ht_insert_element(&vm->fs_global.symbol_table, name, address);
  }
  return NO_ERR;
}

i32 vm_define_function(struct VM_state* vm, const char* name, cfunction func, i32 argc) {
  struct Object value = (struct Object) {
    .value.cfunc = (struct CFunction) {
      .func = func,
      .argc = argc,
    },
    .type = T_CFUNCTION,
  };
  return vm_define_value(vm, name, value);
}

i32 vm_debug_print(struct VM_state* vm) {
  struct Object* value = stack_pop(vm);
  if (value) {
    object_printline(stdout, value);
  }
  return 0;
}

i32 execute(struct VM_state* vm) {
  i32 stack_base = vm->stack_base;
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
        break;
      }
      case I_PUSH_ARG: {
        i32 address = *(vm->ip++);
        i32 index = stack_base + address;
        assert(index <= vm->stack_top);
        struct Object obj = vm->stack[index];
        stack_push(vm, obj);
        break;
      }
      case I_POP:
        break;
      case I_ASSIGN: {
        i32 address = *(vm->ip++);
        struct Object* left = &vm->values[address];
        const struct Object* right = stack_get_top(vm);
        if (!right) {
#if 0
          assert(0);
          return vm->status = ERR;
#else
          left->type = T_UNKNOWN;
          break;
#endif
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
      // TODO(lucas): Clean up I_CALL and I_LOCAL_CALL
      case I_CALL: {
        i32 address = *(vm->ip++);
        assert(address >= 0 && address < vm->values_count);
        struct Object* value = &vm->values[address];
        if (value->type == T_CFUNCTION) {
          i32 argc = value->value.cfunc.argc;
          if (vm->stack_top < argc) {
            runtime_error("Invalid number of arguments in C function call (should be %i)\n", argc);
          }
          i32 base = vm->stack_base = vm->stack_top - argc;
          i32 ret_value_count = value->value.cfunc.func(vm);
          if (ret_value_count > 0) {
            vm->stack[base] = *stack_get_top(vm);
            vm->stack_top = base + 1;
          }
          else {
            vm->stack_top = base - 2;
          }
          break;
        }
        if (value->type != T_FUNCTION) {
          runtime_error("Attempted to call a value which is not a function\n");
          return vm->status = ERR;
        }
        i32 argc = value->value.func.argc;
        if (vm->stack_top < argc) {
          runtime_error("Invalid number of arguments in function call (should be %i)\n", value->value.func.argc);
          return vm->status = ERR;
        }
        i32 old_stack_top = vm->stack_top; // Save the stack pointer so that we can figure out how many return values that the function produced
        i32 base = vm->stack_base = vm->stack_top - argc;

        i32* old_ip = vm->ip; // Save the current instruction pointer position
        vm->ip = &vm->program[value->value.func.address];
        execute(vm);  // Execute function
        vm->ip = old_ip; // Restore the old instruction pointer

        i32 ret_value_count = vm->stack_top - old_stack_top; // TODO(lucas): Implement use of multiple return values
        if (ret_value_count > 0) {
          vm->stack[base] = *stack_get_top(vm);
          vm->stack_top = base + 1;
        }
        else {
          vm->stack_top = base - 2;
        }
        break;
      }
      // n args, i_push <function>, i_local_call <n_args>
      case I_LOCAL_CALL: {
        i32 argc = *(vm->ip++);
        struct Object value = *stack_pop(vm);
        if (value.type == T_CFUNCTION) {
          i32 argc = value.value.cfunc.argc;
          if (vm->stack_top < argc) {
            runtime_error("Invalid number of arguments in C function call (should be %i)\n", argc);
          }
          i32 base = vm->stack_base = vm->stack_top - argc;
          i32 ret_value_count = value.value.cfunc.func(vm);
          if (ret_value_count > 0) {
            vm->stack[base] = *stack_get_top(vm);
            vm->stack_top = base + 1;
          }
          else {
            vm->stack_top = base - 2;
          }
          break;
        }
        if (value.type != T_FUNCTION) {
          runtime_error("Attempted to call a value which is not a function\n");
          return vm->status = ERR;
        }
        i32 func_argc = value.value.func.argc;
        if (vm->stack_top < func_argc && func_argc != argc) {
          runtime_error("Invalid number of arguments in local function call (should be %i)\n", func_argc);
          return vm->status = ERR;
        }

        i32 old_stack_top = vm->stack_top;
        i32 base = vm->stack_base = vm->stack_top - argc;

        i32* old_ip = vm->ip;
        vm->ip = &vm->program[value.value.func.address];
        execute(vm);
        vm->ip = old_ip;

        i32 ret_value_count = vm->stack_top - old_stack_top;
        if (ret_value_count > 0) {
          vm->stack[base] = *stack_get_top(vm);
          vm->stack_top = base + 1;
        }
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
      case I_LT:
        ARITH(vm, <);
        break;
      case I_GT:
        ARITH(vm, >);
        break;
      case I_EQ: {
        if (vm->stack_top >= 2) {
          struct Object* left = stack_get(vm, 1);
          struct Object* right = stack_get(vm, 0);
          i32 result = objects_are_equal(left, right);
          left->value.number = result;
          left->type = T_NUMBER;
          stack_pop(vm);
        }
        else {
          runtime_error("Not enough arguments for arithmetic operation\n");
          vm->status = ERR;
          goto done;
        }
        break;
      }
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
  buffer_free(&vm->buffer);
  func_free(&vm->global);
  func_state_free(&vm->fs_global);
  list_free(vm->program, vm->program_size);
  vm->ip = NULL;
}
