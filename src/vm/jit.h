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

typedef enum {
    ADD_PRIMIT_INT_OP,
    SUB_PRIMIT_INT_OP,
    MUL_PRIMIT_INT_OP,
    DIV_PRIMIT_INT_OP,
    MOD_PRIMIT_INT_OP,
    LT_PRIMIT_INT_OP,
    GT_PRIMIT_INT_OP,
    LE_PRIMIT_INT_OP,
    GE_PRIMIT_INT_OP,
    EQ_PRIMIT_INT_OP,
    GET_PRIMIT_ARRAY_OP,
    SET_PRIMIT_ARRAY_OP,
    LENGTH_PRIMIT_ARRAY_OP,
    CALL_SLOT_INSTANCE_OP,
} PRIMITIVE_OPCODE;

PRIMITIVE_OPCODE get_primitive_opcode (char* op_name);

void jit_diver(ConstantPool* pool, GlobalEnvironment* genv, MethodObj* method);
void* jit_compile_method(MethodObj* method);

void* jit_emit_set_up_call_slot(CodeSection* code_section, void* ra, int nargs, int nlocals, void* method_addr);

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
extern char call_slot_ins[];
extern char call_slot_ins_end[];
extern char primitive_int_op[];
extern char primitive_array_op[];
extern char trap_ins[];
extern char trap_ins_end[];
extern char trap_to_c[];
extern char trap_to_c_end[];
extern char set_up_call_slot[];
extern char set_up_call_slot_end[];
extern char slot_ins[];
extern char slot_ins_end[];
extern char set_slot_ins[];
extern char set_slot_ins_end[];
#endif