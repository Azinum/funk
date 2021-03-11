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

struct Function_state {
  struct Function* func;
  Htable args;
};

struct Ins_desc;

typedef void (*ins_desc_callback)(struct VM_state* vm, struct Ins_desc* ins_desc, i32 instruction, i32 arg_index, FILE* fp);

typedef struct Ins_desc {
  const char* name;
  i32 argc;
  ins_desc_callback callback;
} Ins_desc;

// Code generating functions
static i32 ins_add(struct VM_state* vm, i32 instruction, i32* ins_count);
static i32 value_add(struct VM_state* vm, struct Object obj);
static i32 define_value(struct VM_state* vm, struct Token token, i32* address);
static i32 get_value_address(struct VM_state* vm, struct Token token, i32* address);
static i32 token_to_op(const struct Token* token);
static i32 generate_func(struct VM_state* vm, struct Token name, Ast* params, Ast* body, i32* ins_count);
static i32 generate(struct VM_state* vm, Ast* ast, i32* ins_count);

// Functions for writing byte-code descriptions to files
// TODO(lucas): Seperate this out to another file
static void output_byte_code(struct VM_state* vm, const char* path);
static void desc_push_ins(struct VM_state* vm, struct Ins_desc* ins_desc, i32 instruction, i32 arg_index, FILE* fp);

static Ins_desc ins_desc[MAX_INS] = {
  {"exit",      0,  NULL},
  {"unknown",   0,  NULL},
  {"nop",       0,  NULL},

  {"push",      1,  desc_push_ins},
  {"pop",       0,  NULL},
  {"assign",    1,  NULL},
  {"cond_jump", 1,  NULL},
  {"jump",      1,  NULL},
  {"return",    0,  NULL},
  {"call",      1,  NULL},

  {"add",       0,  NULL},
  {"sub",       0,  NULL},
  {"mul",       0,  NULL},
  {"div",       0,  NULL},
};

void output_byte_code(struct VM_state* vm, const char* path) {
  FILE* fp = fopen(path, "w");
  if (!fp) {
    fprintf(stderr, "Failed to open file '%s'\n", path);
    return;
  }
  for (i32 i = 0; i < vm->program_size; i++) {
    i32 ins = vm->program[i];
    assert(ins >= 0 && ins < MAX_INS);
    Ins_desc desc = ins_desc[ins];
    if (desc.argc > 0) {
      fprintf(fp, "%.4i %-14s", i, desc.name);
      for (i32 arg = 0; arg < desc.argc; arg++) {
        if (desc.callback) {
          desc.callback(vm, &desc, ins, i + arg + 1, fp);
        }
        else {
          fprintf(fp, "%i", vm->program[i + arg + 1]);
        }
        if (arg < (desc.argc - 1)) {
          fprintf(fp, ", ");
        }
      }
      fprintf(fp, "\n");
      i += desc.argc;
    }
    else {
      fprintf(fp, "%.4i %s\n", i, desc.name);
    }
  }
  fclose(fp);
}

void desc_push_ins(struct VM_state* vm, struct Ins_desc* ins_desc, i32 instruction, i32 arg_index, FILE* fp) {
  i32 arg = vm->program[arg_index];
  fprintf(fp, "%i", arg);
  fprintf(fp, " (");
  fprintf(fp, "value = ");

  struct Object* value = &vm->values[arg];
  assert(value);
  object_print(fp, value);
  fprintf(fp, ")");
}

i32 ins_add(struct VM_state* vm, i32 instruction, i32* ins_count) {
  list_push(vm->program, vm->program_size, instruction);
  if (ins_count)
    (*ins_count)++;
  return NO_ERR;
}

i32 value_add(struct VM_state* vm, struct Object obj) {
  i32 address = vm->values_count;
  list_push(vm->values, vm->values_count, obj);
  return address;
}

i32 define_value(struct VM_state* vm, struct Token token, i32* address) {
  struct Function* func = &vm->global;
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(token.string, name, token.length, HTABLE_KEY_SIZE);

  struct Object obj = (struct Object) { .type = T_UNKNOWN, };
  const i32* found = ht_lookup(&func->symbol_table, name);
  if (found) {
    compile_error("Value '%.*s' has already been defined\n", token.length, token.string);
    return ERR;
  }
  else {
    *address = value_add(vm, obj);
    ht_insert_element(&func->symbol_table, name, *address);
  }
  // printf("Define value: '%.*s' -> addr: %i\n", token.length, token.string, *address);
  return NO_ERR;
}

i32 get_value_address(struct VM_state* vm, struct Token token, i32* address) {
  struct Function* func = &vm->global;
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(token.string, name, token.length, HTABLE_KEY_SIZE);

  const i32* found = ht_lookup(&func->symbol_table, name);
  if (!found) {
    compile_error("No such value '%.*s'\n", token.length, token.string);
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

i32 generate_func(struct VM_state* vm, struct Token name, Ast* params, Ast* body, i32* ins_count) {
  struct Function* global = &vm->global;

  // Allocate a new value for this function
  i32 address = -1;
  if (define_value(vm, name, &address) != NO_ERR) {
    return vm->status = ERR;
  }
  assert(address != -1);
  struct Object* func_value = &vm->values[address];
  func_value->type = T_FUNCTION;
  func_init(&func_value->value.func, global);

  // Jump the function body
  ins_add(vm, I_JUMP, ins_count);
  i32 func_jump_ins_index = vm->program_size;
  ins_add(vm, UNRESOLVED_JUMP, ins_count);

  func_value->value.func.address = vm->program_size;

  i32 func_ins_count = 0;

  //
  // TODO(lucas): Parameters
  //

  // Generate the function body
  generate(vm, body, &func_ins_count);
  // Add return instruction at the end of the function
  ins_add(vm, I_RETURN, &func_ins_count);

  list_assign(vm->program, vm->program_size, func_jump_ins_index, func_ins_count);
  *ins_count += func_ins_count;
  return NO_ERR;
}

// TODO(lucas): Think about how error recovery should be done. We don't
// want to exit the interpreter just because we encountered a compile-time
// error. Maybe allocation and initialization of constants/functions e.t.c.
// should be done after we have successfully generated the byte code
// (or in some other way keep track of what new changes the code generator did).
// This is so that in case of an error, we can do a rollback to the previous state.
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
            i32 address = value_add(vm, obj);
            ins_add(vm, I_PUSH, ins_count);
            ins_add(vm, address, ins_count);
          }
          else {
            assert(0);
          }
          break;
        }
        case T_IDENTIFIER: {
          i32 address = -1;
          if (get_value_address(vm, *token, &address) != NO_ERR) {
            return vm->status;
          }
          assert(address != -1);
          struct Object* value = &vm->values[address];
          if (value->type == T_FUNCTION) {
            ins_add(vm, I_CALL, ins_count);
            ins_add(vm, value->value.func.address, ins_count);
          }
          else {
            ins_add(vm, I_PUSH, ins_count);
            ins_add(vm, address, ins_count);
          }
          break;
        }
        case T_LET: {
          i32 address = -1;
          if ((token = ast_get_node_value(ast, ++i))) {
            if (define_value(vm, *token, &address) == NO_ERR) {
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
          Ast cond = ast_get_node_at(&if_branch, 0);
          Ast true_body = ast_get_node_at(&if_branch, 1);
          Ast false_body = ast_get_node_at(&if_branch, 2);
          assert(cond && true_body && false_body);
          generate(vm, &cond, ins_count);

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

          i32 false_body_child_count = ast_child_count(&false_body);
          if (false_body_child_count > 0) {
            // Jump at the end of the if expression body
            ins_add(vm, I_JUMP, ins_count);
            i32 jump_ins_index = vm->program_size;
            ins_add(vm, UNRESOLVED_JUMP, ins_count);

            // Generate the second expression of the if statement
            generate(vm, &false_body, &false_body_ins_count);
            // Resolve jump
            list_assign(vm->program, vm->program_size, jump_ins_index, false_body_ins_count);
            *ins_count += false_body_ins_count;
          }
          // printf("Cond jump: %i, Else jump: %i\n", true_body_ins_count, false_body_ins_count);
          break;
        }
        case T_DEFINE: {
          Ast func = ast_get_node_at(ast, i);
          if ((token = ast_get_node_value(&func, 0))) {
            Ast params = ast_get_node_at(&func, 1);
            Ast body = ast_get_node_at(&func, 2);
            assert(params && body);
            if (generate_func(vm, *token, &params, &body, ins_count) != NO_ERR) {
              return vm->status;
            }
          }
          else {
            assert(0);
          }
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
  ins_add(vm, I_RETURN, &ins_count);
  output_byte_code(vm, "bytecode.txt");
  return result;
}
