#ifndef JIT_H
#define JIT_H
#include "./object.h"
#include "./state.h"

typedef enum {
    EXIT_CODE,
    GC_CODE,
    PRINTF_CODE,
    SLOT_CODE,
    SET_SLOT_CODE,
    CALL_SLOT_CODE
} OPERATION_ID;

void jit_diver(ConstantPool* pool, GlobalEnvironment* genv, MethodObj* method);
void* jit_compile_method(MethodObj* method);

extern char goto_ins[];
extern char goto_ins_end[];
extern char branch_ins[];
extern char branch_ins_end[];
extern char lit_ins[];
extern char lit_ins_end[];
extern char get_local_ins[];
extern char get_local_ins_end[];
extern char set_local_ins[];
extern char set_local_ins_end[];
extern char get_global_ins[];
extern char get_global_ins_end[];
extern char set_global_ins[];
extern char set_global_ins_end[];
extern char drop_ins[];
extern char drop_ins_end[];
extern char return_ins[];
extern char return_ins_end[];
extern char object_ins[];
extern char object_ins_end[];
extern char array_ins[];
extern char array_ins_end[];
extern char call_ins[];
extern char call_ins_end[];
extern char trap_ins[];
extern char trap_ins_end[];
extern char trap_to_c[];
extern char trap_to_c_end[];

#endif