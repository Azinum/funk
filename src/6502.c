// 6502.c

#include "common.h"
#include "list.h"
#include "util.h"
#include "ast.h"
#include "parser.h"
#include "6502_code.h"
#include "6502.h"

static void compile_state_init(struct Compile_state* state);
static void compile_state_free(struct Compile_state* state);
static void output_program(struct Compile_state* state, char* path);

void compile_state_init(struct Compile_state* state) {
  state->status = NO_ERR;
  state->program = NULL;
  state->program_size = 0;
  state->data_section = 0x1;
  ht_create_empty(&state->symbol_table);
}

void compile_state_free(struct Compile_state* state) {
  list_free(state->program, state->program_size);
  ht_free(&state->symbol_table);
}

void output_program(struct Compile_state* state, char* path) {
  FILE* fp = fopen(path, "w");
  if (fp) {
    fwrite(state->program, state->program_size, 1, fp);
    fclose(fp);
  }
}

i32 run_6502(char* path) {
  i32 result = NO_ERR;
  char* source = read_file(path);
  if (source) {
    struct Compile_state state;
    compile_state_init(&state);

    Ast ast = ast_create();
    if (parser_parse(source, path, &ast) == NO_ERR) {
      // ast_print(ast);
      if ((result = code_gen_6502(&state, &ast)) == NO_ERR) {
        char output_path[MAX_PATH_SIZE] = {0};
        snprintf(output_path, MAX_PATH_SIZE, "%s.o65", path);
        output_program(&state, output_path);
      }
    }
    ast_free(&ast);
    free(source);
    compile_state_free(&state);
  }
  else {
    return ERR;
  }
  return result;
}
