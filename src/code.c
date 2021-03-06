// code.c
// Code generator (abstract syntax tree -> byte code)

#include "common.h"
#include "ast.h"
#include "vm.h"
#include "util.h"
#include "error.h"
#include "code.h"

#define compile_error(fmt, ...) \
  fprintf(stderr, "compile-error: " fmt, ##__VA_ARGS__)

#define compile_error2(token, fmt, ...) \
  fprintf(stderr, "compile-error: %s:%i:%i: " fmt, token.filename, token.line, token.count, ##__VA_ARGS__); \
  error_printline((&token.source[0]), token)

#define UNRESOLVED_JUMP 0

struct Ins_desc;

typedef void (*ins_desc_callback)(struct VM_state* vm, struct Ins_desc* ins_desc, i32 instruction, i32 arg_index, FILE* fp);

typedef struct Ins_desc {
  const char* name;
  i32 argc;
  ins_desc_callback callback;
} Ins_desc;

static i32 num_values_added = 0; // How many values was added in this code generation pass?
static i32 old_program_size = 0;  // To know how much of the program that we need to shrink back to in case of a rollback
static Htable symbols;  // Which symbols was added in this code generation pass?

// Code generating functions
static i32 set_branch_type(i32* branch_type, i32 type);
static i32 ins_add(struct VM_state* vm, i32 instruction, i32* ins_count);
static i32 value_add(struct VM_state* vm, struct Object value);
static i32 define_value(struct VM_state* vm, struct Token token, struct Function_state* fs, i32* address);
static i32 define_value_and_type(struct VM_state* vm, struct Token token, struct Function_state* fs, i32 type, i32* address);
static i32 define_arg(struct VM_state* vm, struct Token token, struct Function_state* fs, i32* address);
static i32 get_arg_value_address(struct VM_state* vm, struct Token token, struct Function_state* fs, i32* address);
static i32 get_value_address(struct VM_state* vm, struct Token token, struct Function_state* fs, i32* address);
static i32 token_to_op(const struct Token* token);
static i32 generate_func(struct VM_state* vm, struct Token name, Ast* args, Ast* body, struct Function_state* fs, i32* ins_count);
static i32 generate(struct VM_state* vm, Ast* ast, struct Function_state* fs, i32* ins_count, i32* branch_type);

// Functions for writing byte-code descriptions to files
static void output_byte_code(struct VM_state* vm, const char* path);
static void desc_value_ins(struct VM_state* vm, struct Ins_desc* ins_desc, i32 instruction, i32 arg_index, FILE* fp);

// The order of the instruction descriptors are based on the Instruction enum from code.h.
static Ins_desc ins_desc[MAX_INS] = {
  {"exit",        0,  NULL},
  {"unknown",     0,  NULL},
  {"nop",         0,  NULL},

  {"push",        1,  desc_value_ins},
  {"push_arg",    1,  NULL},
  {"pop",         0,  NULL},
  {"assign",      1,  NULL},
  {"cond_jump",   1,  NULL},
  {"jump",        1,  NULL},
  {"return",      0,  NULL},
  {"call",        1,  desc_value_ins},
  {"local_call",  1,  NULL},

  {"add",         0,  NULL},
  {"sub",         0,  NULL},
  {"mul",         0,  NULL},
  {"div",         0,  NULL},

  {"lt",          0,  NULL},
  {"gt",          0,  NULL},
  {"eq",          0,  NULL},
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
  if (fp != stdout && fp != stderr) {
    fclose(fp);
  }
}

void desc_value_ins(struct VM_state* vm, struct Ins_desc* ins_desc, i32 instruction, i32 arg_index, FILE* fp) {
  i32 arg = vm->program[arg_index];
  fprintf(fp, "%i (value = ", arg);
  struct Object* value = &vm->values[arg];
  assert(value);
  object_print(fp, value);
  fprintf(fp, ")");
}

i32 set_branch_type(i32* branch_type, i32 type) {
  if (branch_type) {
    *branch_type = type;
    return NO_ERR;
  }
  return ERR;
}

i32 ins_add(struct VM_state* vm, i32 instruction, i32* ins_count) {
  list_push(vm->program, vm->program_size, instruction);
  if (ins_count)
    (*ins_count)++;
  return NO_ERR;
}

i32 value_add(struct VM_state* vm, struct Object value) {
  i32 address = vm->values_count;
  list_push(vm->values, vm->values_count, value);
  num_values_added++;
  return address;
}

i32 define_value(struct VM_state* vm, struct Token token, struct Function_state* fs, i32* address) {
  return define_value_and_type(vm, token, fs, T_UNKNOWN, address);
}

i32 define_value_and_type(struct VM_state* vm, struct Token token, struct Function_state* fs, i32 type, i32* address) {
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(name, token.string, token.length, HTABLE_KEY_SIZE);

  struct Object obj = (struct Object) { .type = type, };
  const i32* found = ht_lookup(&fs->symbol_table, name);
  if (found) {
    compile_error2(token, "Value '%.*s' has already been defined\n", token.length, token.string);
    return vm->status = ERR;
  }
  else {
    *address = value_add(vm, obj);
    ht_insert_element(&fs->symbol_table, name, *address);
    if (fs == &vm->fs_global) {
      // NOTE(lucas): Keep track of new global symbols that was added in
      // this code generation pass (to be able to do rollback on the global symbol table in case of error(s))
      ht_insert_element(&symbols, name, *address);
    }
  }
  return NO_ERR;
}

static i32 define_arg(struct VM_state* vm, struct Token token, struct Function_state* fs, i32* address) {
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(name, token.string, token.length, HTABLE_KEY_SIZE);

  const i32* found = ht_lookup(&fs->args, name);
  if (found) {
    compile_error2(token, "Parameter '%.*s' has already been defined\n", token.length, token.string);
    return vm->status = ERR;
  }
  else {
    *address = ht_num_elements(&fs->args);
    ht_insert_element(&fs->args, name, *address);
  }
  return NO_ERR;
}

i32 get_arg_value_address(struct VM_state* vm, struct Token token, struct Function_state* fs, i32* address) {
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(name, token.string, token.length, HTABLE_KEY_SIZE);

  const i32* found = ht_lookup(&fs->args, name);
  if (found) {
    *address = *found;
    return NO_ERR;
  }
  return ERR;
}

i32 get_value_address(struct VM_state* vm, struct Token token, struct Function_state* fs, i32* address) {
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(name, token.string, token.length, HTABLE_KEY_SIZE);

  i32 found_value = 0;
  do {
    const i32* found = ht_lookup(&fs->symbol_table, name);
    if (found) {
      found_value = 1;
      *address = *found;
      break;
    }
    if (fs == &vm->fs_global) {
      break;
    }
  } while ((fs = fs->parent) != NULL);

  if (!found_value) {
    compile_error2(token, "No such value '%.*s'\n", token.length, token.string);
    return vm->status = ERR;
  }
  return NO_ERR;
}

#define OP_CASE(OP) case T_##OP: return I_##OP

i32 token_to_op(const struct Token* token) {
  switch (token->type) {
    OP_CASE(ADD);
    OP_CASE(SUB);
    OP_CASE(MUL);
    OP_CASE(DIV);
    OP_CASE(LT);
    OP_CASE(GT);
    OP_CASE(EQ);
    default:
      break;
  }
  return I_UNKNOWN;
}

i32 generate_func(struct VM_state* vm, struct Token name, Ast* args, Ast* body, struct Function_state* fs, i32* ins_count) {
  i32 status = NO_ERR;
  // Allocate a new value for this function
  i32 address = -1;
  if (define_value(vm, name, fs, &address) != NO_ERR) {
    return vm->status = ERR;
  }
  assert(address != -1);
  struct Object* func_value = &vm->values[address];
  func_value->type = T_FUNCTION;
  func_init(&func_value->value.func, fs->func /* parent */);

  struct Function_state new_fs;
  func_state_init(&new_fs, fs, &func_value->value.func);

  // To skip the function body
  ins_add(vm, I_JUMP, ins_count);
  i32 func_jump_ins_index = vm->program_size;
  ins_add(vm, UNRESOLVED_JUMP, ins_count);

  func_value->value.func.address = vm->program_size;

  i32 func_ins_count = 0;

  // Function arguments
  i32 arg_count = ast_child_count(args);
  for (i32 i = 0; i < arg_count; i++) {
    struct Token* arg = ast_get_node_value(args, i);
    if (arg) {
      if (arg->type != T_IDENTIFIER) {
        compile_error2((*arg), "Expected identifier in function argument list (got '%.*s')\n", arg->length, arg->string);
        status = ERR;
        goto done;
      }
      if ((status = define_arg(vm, *arg, &new_fs, ins_count)) != NO_ERR) {
        goto done;
      }
    }
  }
  func_value->value.func.argc = arg_count;

  // Generate the function body
  generate(vm, body, &new_fs, &func_ins_count, NULL);
  // Add return instruction at the end of the function
  ins_add(vm, I_RETURN, &func_ins_count);

  list_assign(vm->program, vm->program_size, func_jump_ins_index, func_ins_count);
  *ins_count += func_ins_count;
done:
  func_state_free(&new_fs); // Okay, we are done with the compile-time function state for static checks
  return vm->status = status;
}

i32 generate(struct VM_state* vm, Ast* ast, struct Function_state* fs, i32* ins_count, i32* branch_type) {
  assert(ast);
  i32 child_count = ast_child_count(ast);
  struct Token* token = NULL;
  for (i32 i = 0; i < child_count; i++) {
    if ((token = ast_get_node_value(ast, i))) {
      switch (token->type) {
        case T_STRING:
        case T_NUMBER: {
          struct Object obj;
          if (token_to_object(vm, token, &obj) == NO_ERR) {
            set_branch_type(branch_type, obj.type);
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
          i32 push_ins = -1;  // Which push instruction to use, can be either I_PUSH_ARG or I_PUSH.
          struct Object* value = NULL;
          if (get_arg_value_address(vm, *token, fs, &address) == NO_ERR) {
            push_ins = I_PUSH_ARG;
          }
          else if (get_value_address(vm, *token, fs, &address) == NO_ERR) {
            push_ins = I_PUSH;
            value = &vm->values[address];
          }
          else {
            return vm->status = ERR;
          }

          if (value) {
            // Normal function call
            if (value->type == T_FUNCTION || value->type == T_CFUNCTION) {
              Ast args = ast_get_node_at(ast, i + 1);
              if (args) {
                struct Token* args_token = ast_get_value(&args);
                if (args_token) {
                  // Function arguments (if no function arguments are passed, then this is no function call, which is totally fine!)
                  // We might want to, for instance, pass a function value to another function
                  if (args_token->type == T_EXPR) {
                    // printf("I_CALL: %.*s\n", token->length, token->string);
                    if (ast_child_count(&args) > 0) {
                      generate(vm, &args, fs, ins_count, branch_type);
                    }
                    i++;
                    ins_add(vm, I_CALL, ins_count);
                    ins_add(vm, address, ins_count);
                  }
                }
                else {
                  assert(0);
                }
                break;
              }
            }
            set_branch_type(branch_type, value->type);
          }
          else {
            // Local function call
            if (i + 1 < child_count) {
              Ast args = ast_get_node_at(ast, i + 1);
              if (args) {
                struct Token* args_token = ast_get_value(&args);
                if (args_token) {
                  if (args_token->type == T_EXPR) {
                    i32 num_args = ast_child_count(&args);
                    if (num_args > 0) {
                      generate(vm, &args, fs, ins_count, branch_type);
                    }
                    i++;
                    // printf("I_LOCAL_CALL: %.*s\n", token->length, token->string);
                    ins_add(vm, push_ins, ins_count);
                    ins_add(vm, address, ins_count);
                    ins_add(vm, I_LOCAL_CALL, ins_count);
                    ins_add(vm, num_args, ins_count);
                    break;
                  }
                }
              }
            }
          }
          ins_add(vm, push_ins, ins_count);
          ins_add(vm, address, ins_count);
          break;
        }
        // let
        // \-- identifier
        //        \--type (optional)
        //     \-- (expression)
        case T_LET: {
          Ast let_branch = ast_get_node_at(ast, i);
          Ast ident_branch = ast_get_node_at(&let_branch, 0);
          Ast value_branch = ast_get_node_at(&let_branch, 1);
          i32 value_branch_child_count = ast_child_count(&value_branch);
          assert(value_branch_child_count == 1);

          struct Token* ident = ast_get_value(&ident_branch);
          struct Token* type_token = ast_get_node_value(&ident_branch, 0);
          struct Object type_obj = { .type = T_UNKNOWN };
          assert(ident);

          i32 value_address = -1;
          i32 type = T_UNKNOWN;

          // Handle explicit type
          if (type_token) {
            if (type_token->type == T_IDENTIFIER) {
              i32 type_value_address = -1;
              if (get_value_address(vm, *type_token, fs, &type_value_address) == NO_ERR) {
                assert(type_value_address >= 0 && type_value_address < vm->values_count);
                type_obj = vm->values[type_value_address];
                type = type_obj.type;
              }
              else {
                compile_error2((*type_token), "The type '%.*s' is not defined\n", type_token->length, type_token->string);
                return vm->status = ERR;
              }
            }
            else {
              type = type_token->type;
            }
          }

          if (define_value_and_type(vm, *ident, fs, type, &value_address) == NO_ERR) {
            assert(value_address != -1);

            i32 value_branch_type = -1;
            if ((generate(vm, &value_branch, fs, ins_count, &value_branch_type)) == NO_ERR) {
              // Validate equality between value and branch types
              if (type_token) {
                if (type != value_branch_type) {
                  compile_error2((*type_token), "This expression was expected to have type '%.*s'\n", type_token->length, type_token->string);
                  return vm->status = ERR;
                }
              }

              struct Object* value = &vm->values[value_address];
              value->type = value_branch_type;
              ins_add(vm, I_ASSIGN, ins_count);
              ins_add(vm, value_address, ins_count);
              break;
            }
            else {
              return vm->status = ERR;
            }
          }
          else {
            return vm->status = ERR;
          }
          break;
        }
        case T_DEFINE: {
          Ast func = ast_get_node_at(ast, i);
          if ((token = ast_get_node_value(&func, 0))) {
            Ast args = ast_get_node_at(&func, 1);
            Ast body = ast_get_node_at(&func, 2);
            assert(args && body);
            if (generate_func(vm, *token, &args, &body, fs, ins_count) != NO_ERR) {
              return vm->status;
            }
          }
          else {
            assert(0);
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
          generate(vm, &cond, fs, ins_count, branch_type);

          i32 true_body_ins_count = 0;
          i32 false_body_ins_count = 0;

          // Conditional jump at the beginning of the if expression
          ins_add(vm, I_COND_JUMP, &true_body_ins_count);
          i32 cond_jump_ins_index = vm->program_size;
          ins_add(vm, UNRESOLVED_JUMP, &true_body_ins_count);

          // Generate the first expression (the 'true' expression of the if statement)
          generate(vm, &true_body, fs, &true_body_ins_count, branch_type);
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
            generate(vm, &false_body, fs, &false_body_ins_count, branch_type);
            // Resolve jump
            list_assign(vm->program, vm->program_size, jump_ins_index, false_body_ins_count);
            *ins_count += false_body_ins_count;
          }
          break;
        }
        case T_ADD:
        case T_SUB:
        case T_MUL:
        case T_DIV:
        case T_LT:
        case T_GT:
        case T_EQ: {
          i32 op = token_to_op(token);
          assert(op != I_UNKNOWN);
          Ast op_branch = ast_get_node_at(ast, i);
          assert(op_branch);
          if (ast_child_count(&op_branch) < 2) {
            compile_error2((*token), "Missing operands\n");
            return vm->status = ERR;
          }
          generate(vm, &op_branch, fs, ins_count, branch_type);
          ins_add(vm, op, ins_count);
          break;
        }
        case T_EXPR: {
          Ast expr_branch = ast_get_node_at(ast, i);
          if (ast_child_count(&expr_branch) > 0) {
            generate(vm, &expr_branch, fs, ins_count, branch_type);
          }
          break;
        }
        default:
          break;
      }
    }
  }
  return vm->status;
}

i32 code_gen(struct VM_state* vm, Ast* ast) {
  if (ast_is_empty(*ast))
    return NO_ERR;
  num_values_added = 0;
  symbols = ht_create_empty();

  old_program_size = vm->program_size;
  i32 ins_count = 0;
  i32 result = generate(vm, ast, &vm->fs_global, &ins_count, NULL);

  if (result != NO_ERR) { // Error occured, perform rollback
    i32 diff = vm->program_size - old_program_size;
    list_shrink(vm->program, vm->program_size, diff);
    assert(num_values_added <= vm->values_count);
    list_shrink(vm->values, vm->values_count, num_values_added);  // TODO(lucas): Don't only shrink the value list, but also deallocate value contents that need be
    for (i32 i = 0; i < ht_get_size(&symbols); i++) {
      const Hkey* key = ht_lookup_key(&symbols, i);
      if (key) {
        ht_remove_element(&vm->fs_global.symbol_table, *key);
      }
    }
    goto done;
  }
  ins_add(vm, I_RETURN, &ins_count);
  output_byte_code(vm, "bytecode.txt");
done: {
  ht_free(&symbols);
}
  return result;
}
