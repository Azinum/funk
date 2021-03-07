// util.c

#include "common.h"
#include "memory.h"
#include "util.h"

char* read_file(const char* path) {
	u32 buffer_size = 0;
  u32 read_size = 0;
	FILE* file = fopen(path, "rb");
	if (file == NULL) return NULL;

	fseek(file, 0, SEEK_END);
	buffer_size = ftell(file);
	rewind(file);

	char* buffer = (char*)malloc(sizeof(char) * (buffer_size + 1));

	read_size = fread(buffer, sizeof(char), buffer_size, file);
	buffer[read_size] = '\0';

	fclose(file);
	return buffer;
}

i32 string_to_int(char* string, i32 length, i32* value) {
  *value = 0;
  for (i32 i = 0; i < length; i++) {
    char ch = string[i];
    if (ch >= '0' && ch <= '9') {
      *value = *value * 10 + (string[i] - '0');
    }
    else {
      *value = -1;
      return ERR;
    }
  }
  return NO_ERR;
}

i32 string_copy(char* source, char* dest, i32 length, i32 max_length) {
  for (i32 i = 0; i < length && i < max_length; i++) {
    *dest++ = *source++;
  }
  dest[length] = '\0';
  return NO_ERR;
}
