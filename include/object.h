// object.h

#ifndef _OBJECT_H
#define _OBJECT_H

#include "token.h"

typedef struct Object {
  i32 type;
  union {
    i32 number;
  } value;
} Object;

i32 token_to_object(struct Token* t, struct Object* obj);

void object_print(FILE* fp, struct Object* obj);

void object_printline(FILE* fp, struct Object* obj);

#endif
