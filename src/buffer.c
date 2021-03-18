// buffer.c

#include "common.h"
#include "list.h"
#include "util.h"
#include "buffer.h"

void buffer_init(Buffer* buffer) {
  buffer->data = NULL;
  buffer->length = 0;
}

i32 buffer_append(Buffer* buffer, char* string) {
  i32 length = strlen(string);
  return buffer_append_n(buffer, string, length);
}

// TODO(lucas): Error messages
i32 buffer_append_n(Buffer* buffer, char* string, i32 length) {
  if (length == 0) {
    return NO_ERR;
  }
  assert(length > 0);
  if (!buffer->data) {
    buffer->data = list_init(sizeof(char), length);
    if (!buffer->data) {
      return ERR;
    }
    buffer->length = length;
    string_copy2(buffer->data, string, length, length);
  }
  else {
    char* at = &buffer->data[buffer->length];
    i32 old_length = buffer->length; // To see if the reallocation was successful
    list_realloc(buffer->data, buffer->length, buffer->length + length);
    if (old_length == buffer->length) {
      return ERR;
    }
    string_copy2(at, string, length, length);
  }
  return NO_ERR;
}

void buffer_free(Buffer* buffer) {
  if (buffer->data) {
    m_free(buffer->data, buffer->length);
    buffer->length = 0;
  }
}
