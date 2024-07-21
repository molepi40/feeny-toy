#include <sys/mman.h>
#include "../utils/utils.h"
#include "../compiler/bytecode.h"
#include "./state.h"
#include "./jit.h"

extern ConstantPool* pool;
extern GlobalEnvironment* genv;
extern CodeSection* code_section;
extern void* global_slots;


// if method has been compiled, return it
void* jit_compile_method(MethodObj* method) {
    // maybe bug
    if (method->assembly) {
        return method->assembly;
    }
    CodeSection* code_section = make_code_section();
    method->assembly = code_section->code;
    Vector* code = method->code;
    ByteIns* instr;
    for (int i = 0; i < vector_size(code); ++i) {
        instr = (ByteIns*)vector_get(code, i);

        switch (instr->tag) {
            case LABEL_OP: {
                LabelIns* instr1 = (LabelIns*)instr;
                StringObj* label_name = vector_get(pool, instr1->name);
                DEBUG_INFO(label_name->tag != STRING_OBJ, "invalid type of label name");
                LabelObj* label_obj = global_env_get_label(genv, label_name);
                DEBUG_INFO(label_obj->tag != LABEL_OBJ, "invalid type of label");
                label_obj->assembly_addr = code_section->code_pointer;
                // backfill address of goto and branch ins
                label_obj_backfill(label_obj, label_obj->assembly_addr);
                break;
            }

            case GOTO_OP: {
                GotoIns* instr1 = (GotoIns*)instr;
                StringObj* label_name = vector_get(pool, instr1->name);
                DEBUG_INFO(label_name->tag != STRING_OBJ, "invalid type of label name");
                LabelObj* label_obj = global_env_get_label(genv, label_name);
                DEBUG_INFO(label_obj->tag != LABEL_OBJ, "invalid type of label");
                // maybe bug. if label not compiled, add it to backfill list of the label.
                void* goto_target = label_obj->assembly_addr;
                if (goto_target == NULL) {
                    label_obj_add_backfill(label_obj, code_section->code_pointer + 2);
                }                
                jit_emit_ins_movq_begin(code_section, goto_ins, goto_ins_end, (uint64_t)goto_target);

                break;               
            }

            case BRANCH_OP: {
                BranchIns* instr1 = (BranchIns*)instr;
                StringObj* label_name = vector_get(pool, instr1->name);
                DEBUG_INFO(label_name->tag != STRING_OBJ, "invalid type of label name");
                LabelObj* label_obj  = global_env_get_label(genv, label_name);
                DEBUG_INFO(label_obj->tag != LABEL_OBJ, "invalid type of label");
                // maybe bug. if label not compiled, add it to backfill list of the label.
                void* branch_target = label_obj->assembly_addr;
                if (branch_target == NULL) {
                    label_obj_add_backfill(label_obj, code_section->code_pointer + 2);
                }
                jit_emit_ins_movq_begin(code_section, branch_ins, branch_ins_end, (uint64_t)branch_target);
                break;
            }

            case LIT_OP: {
                LitIns* instr1 = (LitIns*)instr;
                Obj* obj = (Obj*)vector_get(pool, instr1->idx);
                // check operand of Lit instr
                DEBUG_INFO(obj->tag != INT_OBJ && obj->tag != NULL_OBJ, "operand object of Lit instr must be int or null.");

                void* literal = NULL;
                if (obj->tag == INT_OBJ) {
                    literal = make_tagged_int(((IntObj*)obj)->value);
                } else if (obj->tag == NULL_OBJ) {
                    literal = make_tagged_null();
                }
                jit_emit_ins_movq_begin(code_section, lit_ins, lit_ins_end, (uint64_t)literal);
                break;           
            }

            case GET_LOCAL_OP: {
                GetLocalIns* instr1 = (GetLocalIns*)instr;
                int64_t index = (int64_t)instr1->idx;
                jit_emit_ins_movq_begin(code_section, get_local_ins, get_local_ins_end, (uint64_t)index);
                break;
            }

            case SET_LOCAL_OP: {
                SetLocalIns* instr1 = (GetLocalIns*)instr;
                int64_t index = (int64_t)instr1->idx;
                jit_emit_ins_movq_begin(code_section, set_local_ins, set_local_ins_end, (uint64_t)index);
                break;
            }

            case GET_GLOBAL_OP: {
                GetGlobalIns* instr1 = (GetGlobalIns*)instr;
                StringObj* name = vector_get(pool, instr1->name);
                DEBUG_INFO(name->tag != STRING_OBJ, "invalid type of variable name. it must be string");
                int64_t index = (int64_t)global_env_get_var_index(genv, name);
                jit_emit_global_ins(code_section, get_global_ins, get_global_ins_end, index);
                break;
            }

            case SET_GLOBAL_OP: {
                SetGlobalIns* instr1 = (SetGlobalIns*)instr;
                StringObj* name = vector_get(pool, instr1->name);
                DEBUG_INFO(name->tag != STRING_OBJ, "invalid type of variable name. it must be string");
                int64_t index = (int64_t)global_env_get_var_index(genv, name);
                jit_emit_global_ins(code_section, set_global_ins, set_global_ins_end, index);
                break;
            }

            case DROP_OP: {
                jit_emit_ins(code_section, drop_ins, drop_ins_end);
                break;
            }

            case RETURN_OP: {
                jit_emit_ins(code_section, return_ins, return_ins_end);
                break;
            }

            case CALL_OP: {
                CallIns* instr1 = (CallIns*)instr;
                StringObj* method_name = vector_get(pool, instr1->name);
                DEBUG_INFO(method_name->tag != STRING_OBJ, "invalid type of method name. it must be string");
                MethodObj* method_obj = global_env_get_method(genv, method_name);

                int nargs = instr1->arity;
                assert(nargs == method_obj->nargs);
                if (method_obj->assembly == NULL) {
                    method_obj->assembly = jit_compile_method(method_obj);
                }
                jit_emit_call_ins(code_section, nargs, method_obj->nlocals, method_obj->assembly);
                break;
            }

            case OBJECT_OP: {
                ObjectIns* instr1 = (ObjectIns*)instr;
                ClassObj* class_obj = (ClassObj*)vector_get(pool, instr1->class);

                int nslots = class_obj->nslots;
                jit_emit_object_ins(code_section, nslots, class_obj);
                break;                
            }

            case ARRAY_OP: {
                jit_emit_array_ins(code_section);
                break;
            }

            case PRINTF_OP: {
                jit_emit_trap_ins(code_section, instr, PRINTF_CODE);
                break;
            }

            case SLOT_OP: {
                jit_emit_trap_ins(code_section, instr, SLOT_CODE);
                break;
            }

            case SET_SLOT_OP: {
                jit_emit_trap_ins(code_section, instr, SET_SLOT_CODE);
                break;
            }

            case CALL_SLOT_OP: {
                jit_emit_trap_ins(code_section, instr, CALL_SLOT_CODE);
                break;
            }
        }
    }
    return method->assembly;
}


void jit_emit_ins_movq_begin(CodeSection* code_section, char* ins, char* ins_end, uint64_t value_64) {
    size_t code_size = ins_end - ins;
    void* code_pointer = code_section->code_pointer;
    DEBUG_INFO(code_pointer + code_size > code_section->code_top, "out of range of code section");
    memcpy(code_pointer, ins, code_size);
    *(uint64_t*)(code_pointer + 2) = value_64;
    code_section->code_pointer += code_size;
    return;    
}

// void jit_emit_ins_mov_begin(char* ins, char* ins_end, uint32_t value_32) {
//     size_t code_size = ins_end - ins;
//     DEBUG_INFO(code_pointer + code_size > code_section->code_top, "out of range of code section");
//     memcpy(code_pointer, ins, code_size);
//     // mov op
//     *(uint32_t*)(code_pointer + 1) = value_32;
//     code_pointer += code_size;
//     return;       
// }

void jit_emit_global_ins(CodeSection* code_section, char* ins, char* ins_end, int index) {
    size_t code_size = ins_end - ins;
    void* code_pointer = code_section->code_pointer;
    DEBUG_INFO(code_pointer + code_size > code_section->code_top, "out of range of code section");
    memcpy(code_pointer, ins, code_size);
    *(uint64_t*)(code_pointer + 2) = (int64_t)index;
    *(uint64_t*)(code_pointer + 12) = (uint64_t)global_slots;
    code_section->code_pointer += code_size;
    return;    
}

void jit_emit_ins(CodeSection* code_section, char* ins, char* ins_end) {
    size_t code_size = ins_end - ins;
    void* code_pointer = code_section->code_pointer;
    memcpy(code_pointer, ins, code_size);
    code_section->code_pointer += code_size;
    return;
}

void jit_emit_call_ins(CodeSection* code_section, int nargs, int nlocals, void* call_addr) {
    size_t code_size = call_ins_end - call_ins;
    void* code_pointer = code_section->code_pointer;
    memcpy(code_pointer, call_ins, code_size);
    // mov ra, nargs and nlocals
    *((uint64_t*)(code_pointer + 2)) = (uint64_t)(code_pointer + code_size);
    *((int64_t*)(code_pointer + 12)) = (int64_t)nargs;
    *((int64_t*)(code_pointer + 22)) = (int64_t)nlocals;
    *((uint64_t*)(code_pointer + 32)) = (uint64_t)call_addr;
    code_section->code_pointer += code_size;
    return;    
}

void jit_emit_object_ins(CodeSection* code_section, int nslots, void* class_obj) {
    size_t code_size = object_ins_end - object_ins;
    void* code_pointer = code_section->code_pointer;
    memcpy(code_pointer, object_ins, code_size);
    // mov nslosts and class_obj
    *((int64_t*)(code_pointer + 2)) = (int64_t)nslots;
    *((uint64_t*)(code_pointer + 30)) = (uint64_t)trap_to_c;
    *((int64_t*)(code_pointer + 51)) = (int64_t)nslots;
    *((uint64_t*)(code_pointer + 61)) = (uint64_t)class_obj;
    code_section->code_pointer += code_size;
    return;    
}

void jit_emit_array_ins(CodeSection* code_section) {
    size_t code_size = array_ins_end - array_ins;
    void* code_pointer = code_section->code_pointer;
    memcpy(code_pointer, array_ins, code_size);
    *((uint64_t*)(code_pointer + 28)) = (uint64_t)trap_to_c;
    code_section->code_pointer += code_size;
    return;
}

void jit_emit_trap_ins(CodeSection* code_section, ByteIns* bytecode_instr, int op_id) {
    size_t code_size = trap_ins_end - trap_ins;
    void* code_pointer = code_section->code_pointer;
    memcpy(code_pointer, trap_ins, code_size);
    *((uint64_t*)(code_pointer + 11)) = (uint64_t)bytecode_instr;
    *((uint64_t*)(code_pointer + 21)) = (uint64_t)trap_to_c;
    *((uint64_t*)(code_pointer + 35)) = (int64_t)op_id;
    code_section->code_pointer += code_size;
    return;
}