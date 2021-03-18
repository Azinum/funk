// object.c

#include "common.h"
#include "util.h"
#include "hash.h"
#include "vm.h"
#include "object.h"

i32 token_to_object(struct VM_state* vm, struct Token* t, struct Object* obj) {
  switch (t->type) {
    case T_NUMBER: {
      obj->type = T_NUMBER;
      obj->value.number = t->value.number;
      break;
    }
    case T_STRING: {
      if (buffer_append_n(&vm->buffer, t->string, t->length) == NO_ERR) {
        obj->type = T_STRING;
        obj->value.buffer.data = &vm->buffer.data[vm->buffer.length - t->length];
        obj->value.buffer.length = t->length;
      }
      else {
        assert(0);  // TODO(lucas): Handle
      }
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
    case T_STRING: {
      if (obj->value.buffer.data) {
        fprintf(fp, "\"%.*s\"", obj->value.buffer.length, obj->value.buffer.data);
      }
      else {
        fprintf(fp, "\"\"");
      }
      break;
    }
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
