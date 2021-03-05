// funk.c

#include "memory.h"
#include "vm.h"
#include "funk.h"

i32 funk_start(i32 argc, char** argv) {
  char* source = "(+ 2 3)";
  char* file = "stdin";
  struct VM_state vm;
  vm_init(&vm);
  vm_exec(&vm, file, source);
  vm_free(&vm);
  assert(memory_total() == 0);
  return NO_ERR;
}
