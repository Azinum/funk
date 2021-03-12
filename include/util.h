// util.h

#ifndef _UTIL_H
#define _UTIL_H

char* read_file(const char* path);

i32 string_to_int(char* string, i32 length, i32* value);

i32 string_copy(char* source, char* dest, i32 length, i32 max_length);

// String copy, but with no null termination
i32 string_copy2(char* source, char* dest, i32 length, i32 max_length);

#endif
