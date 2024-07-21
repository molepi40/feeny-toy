#ifndef VM_H
#define VM_H
#include "../compiler/bytecode.h"

void interpret_bc (Program* prog, int jit_flag);

#endif
