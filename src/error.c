// error.c

#include "common.h"
#include "token.h"
#include "error.h"

void error_printline(char* source, struct Token token) {
  FILE* fp = stdout;
  char* start = &source[0];
  char* at = token.string;
  i64 at_size = at - start;
  i64 start_index = at_size;

  // Scan to beginning of line
  while (start_index >= 0) {
    if (*at == '\n' || *at == '\r') {
      break;
    }
    at--;
    start_index--;
  }

  i64 end_index = start_index;
  char* end_at = ++at;

  // Scan to end of line
  while (1) {
    if (*end_at == '\n' || *end_at == '\r' || *end_at == '\0') {
      break;
    }
    end_index++;
    end_at++;
  }
  i64 line_size = end_index - start_index;
  i64 pointer_size = at_size - start_index - 1;
  fprintf(fp, "%.*s\n", (i32)(line_size), at);
  for (i32 i = 0; i < pointer_size; i++) {
    fprintf(fp, "-");
  }
  fprintf(fp, "^\n\n");
}
