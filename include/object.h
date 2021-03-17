// object.h

#ifndef _OBJECT_H
#define _OBJECT_H

#include "token.h"
#include "hash.h"
#include "buffer.h"

struct VM_state;

struct Function {
  i32 address;
  i32 argc;
  struct Function* parent;
};

struct Function_state {
  struct Function* func;
  struct Function_state* parent;
  Htable symbol_table;
  Htable args;
};

typedef struct Object {
  union {
    i32 number;
    struct Function func;
    struct Buffer buffer;
    // struct Buffer* buffer;
  } value;
  i32 type;
} Object;

i32 token_to_object(struct VM_state* vm, struct Token* t, struct Object* obj);

void object_print(FILE* fp, struct Object* obj);

void object_printline(FILE* fp, struct Object* obj);

void func_init(struct Function* func, struct Function* parent);

void func_free(struct Function* func);

void func_state_init(struct Function_state* fs, struct Function_state* parent, struct Function* func);

void func_state_free(struct Function_state* fs);

#endif
