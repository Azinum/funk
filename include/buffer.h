// buffer.h

#ifndef _BUFFER_H
#define _BUFFER_H

typedef struct Buffer {
  char* data;
  i32 length;
} Buffer;

typedef struct Buffer_list {
  struct Buffer* buffers;
  i32 length;
} Buffer_list;

void buffer_init(Buffer* buffer);

i32 buffer_append(Buffer* buffer, char* string);

i32 buffer_append_n(Buffer* buffer, char* string, i32 length);

void buffer_free(Buffer* buffer);

#endif
