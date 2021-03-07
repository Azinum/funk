// code.c
// Code generator (abstract syntax tree -> byte code)

#include "common.h"
#include "ast.h"
#include "vm.h"
#include "util.h"
#include "code.h"

// TODO(lucas): Add at which location (line, file) the error occured
#define compile_error(fmt, ...) \
  fprintf(stderr, "compile-error: " fmt, ##__VA_ARGS__)

#define UNRESOLVED_JUMP 0

static i32 ins_add(struct VM_state* vm, i32 instruction, i32* ins_count);
static i32 constant_add(struct VM_state* vm, struct Object obj);
static i32 declare_symbol(struct VM_state* vm, struct Token token, i32* address);
static i32 get_symbol_address(struct VM_state* vm, struct Token token, i32* address);
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

i32 declare_symbol(struct VM_state* vm, struct Token token, i32* address) {
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(token.string, name, token.length, HTABLE_KEY_SIZE);

  struct Object obj = (struct Object) { .type = T_UNKNOWN, };
  const i32* found = ht_lookup(&vm->symbol_table, name);
  if (found) {
    compile_error("Symbol '%.*s' has already been defined\n", token.length, token.string);
    return ERR;
  }
  else {
    *address = constant_add(vm, obj);
    ht_insert_element(&vm->symbol_table, name, *address);
  }
  return NO_ERR;
}

i32 get_symbol_address(struct VM_state* vm, struct Token token, i32* address) {
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(token.string, name, token.length, HTABLE_KEY_SIZE);

  const i32* found = ht_lookup(&vm->symbol_table, name);
  if (!found) {
    compile_error("No such symbol '%.*s'\n", token.length, token.string);
    return vm->status = ERR;
  }
  *address = *found;
  return NO_ERR;
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
            assert(0);
          }
          break;
        }
        case T_IDENTIFIER: {
          i32 address = -1;
          if (get_symbol_address(vm, *token, &address) != NO_ERR) {
            return vm->status;
          }
          assert(address != -1);
          ins_add(vm, I_PUSH, ins_count);
          ins_add(vm, address, ins_count);
          break;
        }
        case T_LET: {
          i32 address = -1;
          if ((token = ast_get_node_value(ast, ++i))) {
            if (declare_symbol(vm, *token, &address) == NO_ERR) {
              assert(address != -1);
              Ast decl_branch = ast_get_node_at(ast, i);
              if (decl_branch) {
                generate(vm, &decl_branch, ins_count);
                ins_add(vm, I_ASSIGN, ins_count);
                ins_add(vm, address, ins_count);
                break;
              }
              else {
                assert(0);
              }
            }
            else {
              return vm->status = ERR;
            }
          }
          else {
            compile_error("Missing identifier in declaration\n");
            return vm->status = ERR;
          }
          break;
        }
        case T_IF: {
          Ast if_branch = ast_get_node_at(ast, i);
          assert(if_branch);
          Ast true_body = ast_get_node_at(&if_branch, 0);
          Ast false_body = ast_get_node_at(&if_branch, 1);
          assert(true_body && false_body);
          i32 true_body_ins_count = 0;
          i32 false_body_ins_count = 0;

          // Conditional jump at the beginning of the if expression
          ins_add(vm, I_COND_JUMP, &true_body_ins_count);
          i32 cond_jump_ins_index = vm->program_size;
          ins_add(vm, UNRESOLVED_JUMP, &true_body_ins_count);

          // Generate the first expression (the 'true' expression of the if statement)
          generate(vm, &true_body, &true_body_ins_count);
          // Resolve jump
          list_assign(vm->program, vm->program_size, cond_jump_ins_index, true_body_ins_count);
          *ins_count += true_body_ins_count;

          // Jump at the end of the if expression body
          ins_add(vm, I_JUMP, ins_count);
          i32 jump_ins_index = vm->program_size;
          ins_add(vm, UNRESOLVED_JUMP, ins_count);

          // Generate the second expression of the if statement
          generate(vm, &false_body, &false_body_ins_count);
          // Resolve jump
          list_assign(vm->program, vm->program_size, jump_ins_index, false_body_ins_count);
          *ins_count += false_body_ins_count;

          // printf("Cond jump: %i, Else jump: %i\n", true_body_ins_count, false_body_ins_count);
          break;
        }
        case T_ADD:
        case T_SUB:
        case T_MUL:
        case T_DIV: {
          i32 op = token_to_op(token);
          assert(op != I_UNKNOWN);
          Ast op_branch = ast_get_node_at(ast, i);
          assert(op_branch);
          if (ast_child_count(&op_branch) < 2) {
            compile_error("Missing operands\n");
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
  if (ast_is_empty(*ast))
    return NO_ERR;
  i32 ins_count = 0;
  i32 result = generate(vm, ast, &ins_count);
  ins_add(vm, I_EXIT, &ins_count);
  return result;
}
