// object.h

#ifndef _OBJECT_H
#define _OBJECT_H

#include "token.h"
#include "hash.h"

struct Function {
  i32 address;
  i32 argc;
  Htable symbol_table;
  struct Function* parent;
};

typedef struct Object {
  i32 type;
  union {
    i32 number;
    struct Function func;
  } value;
} Object;

i32 token_to_object(struct Token* t, struct Object* obj);

void object_print(FILE* fp, struct Object* obj);

void object_printline(FILE* fp, struct Object* obj);

void func_init(struct Function* func, struct Function* parent);

void func_free(struct Function* func);

#endif
