#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/mman.h>
#include "../utils/utils.h"
#include "./state.h"
#include "./object.h"
#include "./mem.h"

/**
 * Global environment operations
 */

GlobalEnvironment* make_global_env() {
    GlobalEnvironment* genv = (GlobalEnvironment*)malloc(sizeof(GlobalEnvironment));
    genv->var_ht = make_hash_table();
    genv->method_ht = make_hash_table();
    genv->label_ht = make_hash_table();
    return genv;
}

void* make_global_slots(int nslots) {
    int slots_size = MIN_OBJ_SIZE + nslots * ADDR_WIDTH;
    void* slots = malloc(slots_size);
    memset(slots, 0, slots_size);

    OBJ_TAG(slots) = ARRAY_HEAP_OBJ;
    GET_SIZE(slots) = slots_size;
    GET_ARRAY_LENGTH(slots) = nslots;

    return slots;
}

void global_env_add_var(GlobalEnvironment* genv, SlotObj* var, int index) {
    hash_table_insert(genv->var_ht, var->name->value, (void*)index);
}

void global_env_add_method(GlobalEnvironment* genv, MethodObj* method) {
    hash_table_insert(genv->method_ht, method->name->value, (void*)method);
}

void global_env_add_label(GlobalEnvironment* genv, StringObj* name, LabelObj* label) {
    hash_table_insert(genv->label_ht, name->value, (void*)label);
}

int global_env_get_var_index(GlobalEnvironment* genv, StringObj* name) {
    HashEntry* entry = hash_table_get(genv->var_ht, name->value);
    int index = (int)entry->value;
    return index;
}

MethodObj* global_env_get_method(GlobalEnvironment* genv, StringObj* name) {
    HashEntry* entry = hash_table_get(genv->method_ht, name->value);
    return (MethodObj*)entry->value;
}

LabelObj* global_env_get_label(GlobalEnvironment* genv, StringObj* name) {
    HashEntry* entry = hash_table_get(genv->label_ht, name->value);
    return (LabelObj*)entry->value;
}

void global_slots_set(void* global_slots, int index, void* value) {
    int length = GET_ARRAY_LENGTH(global_slots);
    DEBUG_INFO(index >= length, "global slots out of range");
    GET_ARRAY_DATA(global_slots, index) = value;
}

void* global_slots_get(void* global_slots, int index) {
    int length = GET_ARRAY_LENGTH(global_slots);
    DEBUG_INFO(index >= length, "global slots out of range");
    return GET_ARRAY_DATA(global_slots, index);    
}

void print_global_variables(void* global_slots) {
    void* var_value;
    int nslots = GET_ARRAY_LENGTH(global_slots);
    for (int i = 0; i < nslots; ++i) {
        var_value = GET_ARRAY_DATA(global_slots, i);
        printf("\tglobal %d: %p\n", i, var_value);
        if (var_value != NULL && OBJ_TAG(var_value) == ARRAY_HEAP_OBJ) {
            print_array(var_value);
        }
    }
}


/**
 * Frame stack operations
 */

FrameStack* make_frame_stack() {
    FrameStack* frame_stack = (FrameStack*)malloc(sizeof(FrameStack));
    frame_stack->stack = (void*)malloc(FRAME_STACK_SIZE);
    frame_stack->stack_top = frame_stack->stack + FRAME_STACK_SIZE;
    return frame_stack;
}

void* make_local_frame(void* parent, AddressObj* ra, int nslots) {
    int local_frame_size = 24 + nslots * ADDR_WIDTH;
    DEBUG_INFO(frame_pointer + local_frame_size > frame_stack->stack_top, "run out of frame stack memory");

    void* frame = frame_pointer;
    memset(frame, 0, local_frame_size);
    GET_FRAME_PARENT(frame) = parent;
    GET_FRAME_RA(frame) = ra;
    GET_FRAME_NSLOTS(frame) = nslots;
    frame_base_pointer = frame;
    frame_pointer = frame + local_frame_size;
    
    return frame;
}

void local_frame_slot_set(void* local_frame, int index, void* value) {
    GET_FRAME_SLOT(local_frame, index) = value;
}

void* local_frame_slot_get(void* local_frame, int index) {
    return GET_FRAME_SLOT(local_frame, index);
}

void print_stack_local_frame(void* local_frame) {
    int nslots;
    void* var_value;
    while (local_frame) {
        nslots = GET_FRAME_NSLOTS(local_frame);
        for (int i = 0; i < nslots; ++i) {
            var_value = GET_FRAME_SLOT(local_frame, i);
            printf("\tlocal %d: %p\n", i, var_value);
            if (var_value != NULL && OBJ_TAG(var_value) == ARRAY_HEAP_OBJ) {
                print_array(var_value);
           }
        }
        local_frame = GET_FRAME_PARENT(local_frame);
    }
}


/**
 * Operand stack operations
 */

OperandStack* make_operand_stack() {
    OperandStack* operand_stack = (OperandStack*)malloc(sizeof(OperandStack));
    operand_stack->stack = malloc(OPERAND_STACK_SIZE);
    operand_stack->stack_top = operand_stack->stack + OPERAND_STACK_SIZE;
    return operand_stack;
}

void* operand_stack_peek() {
    DEBUG_INFO(operand_pointer - ADDR_WIDTH < operand_stack->stack, "operand stack empty");
    return GET_STACK_VALUE(operand_pointer - ADDR_WIDTH);
}

void operand_stack_add(void* value) {
    DEBUG_INFO(operand_pointer + ADDR_WIDTH > operand_stack->stack_top, "run out of operand stack memory");
    GET_STACK_VALUE(operand_pointer) = value;
    operand_pointer += ADDR_WIDTH;
}

void* operand_stack_pop() {
    DEBUG_INFO(operand_pointer - ADDR_WIDTH < operand_stack->stack, "operand stack empty");
    operand_pointer -= ADDR_WIDTH;
    void* value = GET_STACK_VALUE(operand_pointer);
    return value;
}

void print_operand_stack(void* operand_pointer) {
    void* p = operand_stack->stack;
    void* value;
    int i = 0;
    while (p < operand_pointer) {
        value = GET_STACK_VALUE(p);
        printf("\tstack %d: %p\n", i++, value);
        if (value != NULL && OBJ_TAG(value) == ARRAY_HEAP_OBJ) {
            print_array(value);
        }
        p += ADDR_WIDTH;
    }
}


/**
 * Code section
 */

// make code section of size of 1MB
CodeSection* make_code_section() {
    CodeSection* code_section = (CodeSection*)malloc(sizeof(CodeSection));
    code_section->code = mmap(NULL, CODE_SECTION_SIZE,
                              PROT_READ | PROT_WRITE | PROT_EXEC,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(code_section->code, 0, CODE_SECTION_SIZE);
    code_section->code_pointer = code_section->code;
    code_section->code_top = code_section->code + CODE_SECTION_SIZE;
    return code_section;
}