// code.c
// Code generator (abstract syntax tree -> byte code)

#include "common.h"
#include "ast.h"
#include "vm.h"
#include "code.h"

// TODO(lucas): Add at which location (line, file) the error occured
#define compile_error(fmt, ...) \
  fprintf(stderr, "compile-error: " fmt ##__VA_ARGS__)

static i32 ins_add(struct VM_state* vm, i32 instruction, i32* ins_count);
static i32 constant_add(struct VM_state* vm, struct Object obj);
static i32 token_to_op(const struct Token* token);
static i32 generate(struct VM_state* vm, Ast* ast, i32* ins_count);

i32 ins_add(struct VM_state* vm, i32 instruction, i32* ins_count) {
  list_push(vm->program, vm->program_size, instruction);
  if (ins_count)
    (*ins_count)++;
  return NO_ERR;
}

i32 constant_add(struct VM_state* vm, struct Object obj) {
  i32 address = vm->constant_count;
  list_push(vm->constants, vm->constant_count, obj);
  return address;
}

#define OP_CASE(OP) case T_##OP: return I_##OP

i32 token_to_op(const struct Token* token) {
  switch (token->type) {
    OP_CASE(ADD);
    OP_CASE(SUB);
    OP_CASE(MUL);
    OP_CASE(DIV);
    default:
      break;
  }
  return I_UNKNOWN;
}

// TODO(lucas): Think about how error recovery should be done. We don't want to exit the interpreter just because we encountered a compile-time error. Maybe allocation and initialization of constants/functions e.t.c. should be done after we have successfully generated the byte code (or at least in some way keep track of what new changes the code generator did). This is so that in case of an error, we can do a rollback to the previous state.
i32 generate(struct VM_state* vm, Ast* ast, i32* ins_count) {
  assert(ast);
  i32 child_count = ast_child_count(ast);
  struct Token* token = NULL;
  for (i32 i = 0; i < child_count; i++) {
    if ((token = ast_get_node_value(ast, i))) {
      switch (token->type) {
        case T_NUMBER: {
          struct Object obj;
          if (token_to_object(token, &obj) == NO_ERR) {
            i32 constant = constant_add(vm, obj);
            ins_add(vm, I_PUSH, ins_count);
            ins_add(vm, constant, ins_count);
          }
          else {
            // TODO(lucas): Handle, give error.
          }
          break;
        }
        case T_ADD:
        case T_SUB:
        case T_MUL:
        case T_DIV: {
          i32 op = token_to_op(token);
          assert(op != I_UNKNOWN);
          Ast op_branch = ast_get_node_at(ast, 0);
          if (!op_branch) {
            // This really should not happen, amirite?
            compile_error("Missing operator branch\n");
            return ERR;
          }
          generate(vm, &op_branch, ins_count);
          ins_add(vm, op, ins_count);
          break;
        }
        default:
          break;
      }
    }
  }
  return NO_ERR;
}

i32 code_gen(struct VM_state* vm, Ast* ast) {
  i32 ins_count = 0;
  i32 result = generate(vm, ast, &ins_count);
  ins_add(vm, I_EXIT, &ins_count);
  return result;
}

