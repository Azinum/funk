// object.c

#include "common.h"
#include "util.h"
#include "hash.h"
#include "object.h"

i32 token_to_object(struct Token* t, struct Object* obj) {
  switch (t->type) {
    case T_NUMBER: {
      obj->type = T_NUMBER;
      obj->value.number = 0;
      string_to_int(t->string, t->length, &obj->value.number);
      break;
    }
    default:
      assert(0);
      break;
  }
  return NO_ERR;
}

void object_print(FILE* fp, struct Object* obj) {
  switch (obj->type) {
    case T_NUMBER: {
      fprintf(fp, "%i", obj->value.number);
      break;
    }
    case T_FUNCTION: {
      fprintf(fp, "function: %i", obj->value.func.address);
      break;
    }
    default:
      fprintf(fp, "?");
      break;
  }
}

void object_printline(FILE* fp, struct Object* obj) {
  object_print(fp, obj);
  fprintf(fp, "\n");
}

void func_init(struct Function* func, struct Function* parent) {
  func->argc = 0;
  func->address = 0;
  func->parent = parent;
}

void func_free(struct Function* func) {
  (void)func;
}

void func_state_init(struct Function_state* fs, struct Function_state* parent, struct Function* func) {
  fs->func = func;
  fs->parent = parent;
  fs->symbol_table = ht_create_empty();
  fs->args = ht_create_empty();
}

void func_state_free(struct Function_state* fs) {
  fs->func = NULL;
  ht_free(&fs->symbol_table);
  ht_free(&fs->args);
}
