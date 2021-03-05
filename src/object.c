// object.c

#include "common.h"
#include "util.h"
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
    default:
      break;
  }
}

void object_printline(FILE* fp, struct Object* obj) {
  object_print(fp, obj);
  fprintf(fp, "\n");
}
