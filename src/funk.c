// funk.c

#include "memory.h"
#include "vm.h"
#include "util.h"
#include "funk.h"

#define MAX_INPUT 512
#define PROMPT "> "
#define readinput(buffer) (fprintf(stdout, "%s", PROMPT), fgets(buffer, MAX_INPUT, stdin) != NULL)

static i32 user_input(struct VM_state* vm);

i32 funk_start(i32 argc, char** argv) {
  struct VM_state vm;
  vm_init(&vm);
#if 0
  user_input(&vm);
#else
  char* path = "test.funk";
  char* source = read_file(path);
  if (source) {
    vm_exec(&vm, path, source);
    free(source);
  }
#endif
  vm_free(&vm);
  assert(memory_total() == 0);
  return NO_ERR;
}

i32 user_input(struct VM_state* vm) {
  i32 status = NO_ERR;
  char input[MAX_INPUT] = {0};
  char* buffer = input;
  char* file = "stdin";
  while (1) {
    if (readinput(buffer)) {
      if ((status = vm_exec(vm, file, buffer)) != NO_ERR) {
        return status;
      }
    }
    else {
      break;
    }
  }
  return NO_ERR;
}
