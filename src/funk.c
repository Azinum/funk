// funk.c

#include "memory.h"
#include "vm.h"
#include "funk.h"

i32 funk_start(i32 argc, char** argv) {
  char* source = "(let a (+ 2 3)) (let a (+ a 5)) (a)";
  char* file = "stdin";
  struct VM_state vm;
  vm_init(&vm);
  vm_exec(&vm, file, source);
  vm_free(&vm);
  assert(memory_total() == 0);
  return NO_ERR;
}
