// 6502_code.c

#include "common.h"
#include "util.h"
#include "list.h"
#include "token.h"
#include "ast.h"
#include "error.h"
#include "6502.h"
#include "6502_code.h"

#define compile_error(token, fmt, ...) \
  fprintf(stderr, "compile-error: %s:%i:%i: " fmt, token.filename, token.line, token.count, ##__VA_ARGS__); \

#define compile_error2(token, fmt, ...) \
  fprintf(stderr, "compile-error: %s:%i:%i: " fmt, token.filename, token.line, token.count, ##__VA_ARGS__); \
  error_printline((&token.source[0]), token)

static i32 alloc_byte(struct Compile_state* state, i32* address);
static i32 define_value(struct Compile_state* state, struct Token token, i32 type, i32* address);
static i32 get_value_address(struct Compile_state* state, struct Token token, i32* address);
static i32 set_branch_type(i32* branch_type, i32 type);
static i32 ins_add(struct Compile_state* state, i8 instruction, i32* ins_count);
static i32 generate(struct Compile_state* state, Ast* ast, i32* ins_count, i32* branch_type);

// Stores stuff both in the zero page and non-zero page, but only zero page memory is used at the moment
i32 alloc_byte(struct Compile_state* state, i32* address) {
  *address = state->data_section;
  state->data_section += sizeof(i8);
  return NO_ERR;
}

i32 define_value(struct Compile_state* state, struct Token token, i32 type, i32* address) {
  char name[HTABLE_KEY_SIZE] = {0};
  string_copy(name, token.string, token.length, HTABLE_KEY_SIZE);
  const i32* found = ht_lookup(&state->symbol_table, name);
  if (found) {
    compile_error2(token, "Value '%.*s' has already been defined\n", token.length, token.string);
    return state->status = ERR;
  }
  else {
    switch (type) {
      case T_NUMBER: {
        alloc_byte(state, address);
        break;
      }
      default:
        assert(0);
        break;
    }
    ht_insert_element(&state->symbol_table, name, *address);
  }
  return NO_ERR;
}

i32 get_value_address(struct Compile_state* state, struct Token token, i32* address) {
  char name[HTABLE_KEY_SIZE] = {};
  string_copy(name, token.string, token.length, HTABLE_KEY_SIZE);
  const i32* found = ht_lookup(&state->symbol_table, name);
  if (!found) {
    compile_error2(token, "No such value '%.*s'\n", token.length, token.string);
    return state->status = ERR;
  }
  else {
    *address = *found;
  }
  return NO_ERR;
}

i32 set_branch_type(i32* branch_type, i32 type) {
  if (branch_type) {
    *branch_type = type;
    return NO_ERR;
  }
  return ERR;
}

i32 ins_add(struct Compile_state* state, i8 instruction, i32* ins_count) {
  list_push(state->program, state->program_size, instruction);
  if (ins_count) {
    (*ins_count)++;
  }
  return NO_ERR;
}

i32 generate(struct Compile_state* state, Ast* ast, i32* ins_count, i32* branch_type) {
#if 0
  ins_add(state, OP_LDA_IMM, ins_count);
  ins_add(state, 1, ins_count);
  ins_add(state, OP_ADC_IMM, ins_count);
  ins_add(state, 2, ins_count);
  ins_add(state, OP_LDX_IMM, ins_count);
  ins_add(state, 5, ins_count);

  ins_add(state, OP_LDA_ZPG_X, ins_count);
  ins_add(state, 3, ins_count);
#endif

#if 1
  i32 child_count = ast_child_count(ast);
  struct Token* token = NULL;
  for (i32 i = 0; i < child_count; i++) {
    if ((token = ast_get_node_value(ast, i))) {
      switch (token->type) {
        // Load value into A
        case T_NUMBER: {
          set_branch_type(branch_type, T_NUMBER);
          i8 value = (i8)token->value.number;
          ins_add(state, OP_LDA_IMM, ins_count);
          ins_add(state, value, ins_count);
          break;
        }
        // Load value into A
        case T_IDENTIFIER: {
          i32 value_address = -1;
          if (get_value_address(state, *token, &value_address) == NO_ERR) {
            if (value_address <= INT8_MAX) {
              i8 address = (i8)value_address;
              ins_add(state, OP_LDA_ZPG, ins_count);
              ins_add(state, address, ins_count);
            }
            else {
              // Handle
            }
          }
          else {
            return state->status;
          }
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
          struct Token* ident = ast_get_value(&ident_branch);
          struct Token* type_token = ast_get_node_value(&ident_branch, 0);
          if (type_token) {
            if (type_token->type == T_NUMBER) {
              i32 value_branch_type = -1;
              if ((generate(state, &value_branch, ins_count, &value_branch_type)) == NO_ERR) {
                if (type_token->type == value_branch_type) {
                  i32 value_address = -1;
                  if (define_value(state, *ident, type_token->type, &value_address) == NO_ERR) {
                    if (value_address <= INT8_MAX) { // Zero page mode only allows for addresses up to INT8_MAX i.e. 0-255
                      i8 address = (i8)value_address;
                      // Store the value of A into address
                      ins_add(state, OP_STA_ZPG, ins_count);
                      ins_add(state, address, ins_count);
                    }
                    else {
                      // Handle
                    }
                  }
                  else {
                    return state->status;
                  }
                }
                else {
                  compile_error((*type_token), "This expression was expected to have type '%.*s'\n", type_token->length, type_token->string);
                  return state->status = ERR;
                }
              }
              else {
                return state->status = ERR;
              }
            }
            else {
              compile_error2((*type_token), "The type '%.*s' is not defined\n", type_token->length, type_token->string);
            }
          }
          else {
            compile_error2((*ident), "Expected type in value definition\n");
            return state->status = ERR;
          
          }
          break;
        }
        case T_EXPR: {
          Ast expr_branch = ast_get_node_at(ast, i);
          if (ast_child_count(&expr_branch) > 0) {
            generate(state, &expr_branch, ins_count, branch_type);
          }
          break;
        }
        default:
          break;
      }
    }
  }
#endif
  return state->status;
}

i32 code_gen_6502(struct Compile_state* state, Ast* ast) {
  i32 result = NO_ERR;
  i32 ins_count = 0;
  i32 branch_type = 0;
  if (ast_is_empty(*ast)) {
    return NO_ERR;
  }
  result = generate(state, ast, &ins_count, &branch_type);
  return result;
}
