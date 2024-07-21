#ifndef STATE_H
#define STATE_H
#include "../utils/utils.h"
#include "../utils/hash.h"
#include "../compiler/bytecode.h"
#include "./object.h"


// frame stack size = 1048576B = 1MB
#define FRAME_STACK_SIZE 1048576

// operand stack size = 1048576B = 1MB
#define OPERAND_STACK_SIZE 1048576

// code section size = 1048576B = 1MB
#define CODE_SECTION_SIZE 1048576

// frame ops
#define GET_FRAME_PARENT(frame) *((void**)(frame))
#define GET_FRAME_RA(frame) *((AddressObj**)(frame + 8))
#define GET_FRAME_NSLOTS(frame) *((int*)(frame + 16))
#define GET_FRAME_SLOT(frame, idx) *((void**)(frame + 24 + idx * ADDR_WIDTH))

// stack ops
#define GET_STACK_VALUE(sp) *((void**)(sp))

// code section ops
#define GET_CODE_VALUE(cp) *((ByteIns**)(cp))

typedef Vector ConstantPool;

typedef struct {
    HashTable* var_ht;
    HashTable* method_ht;
    HashTable* label_ht;
} GlobalEnvironment;

typedef struct {
    void* stack;
    void* stack_top;
} FrameStack;

typedef struct {
    void* stack;
    void* stack_top;
} OperandStack;

typedef struct {
    void* code;
    void* code_top;
    void* code_pointer;
} CodeSection;


// global environment
GlobalEnvironment* make_global_env();

void global_env_add_var(GlobalEnvironment* genv, SlotObj* var, int index);
void global_env_add_method(GlobalEnvironment* genv, MethodObj* method);
void global_env_add_label(GlobalEnvironment* genv, StringObj* name, LabelObj* label);

int global_env_get_var_index(GlobalEnvironment* genv, StringObj* name);
MethodObj* global_env_get_method(GlobalEnvironment* genv, StringObj* name);
LabelObj* global_env_get_label(GlobalEnvironment* genv, StringObj* name);

void* make_global_slots(int nslots);
void global_slots_set(void* global_slots, int index, void* value);
void* global_slots_get(void* global_slots, int index);
void print_global_variables(void* global_slots);


// frame
extern FrameStack* frame_stack;
extern void* frame_base_pointer;
extern void* frame_pointer;

FrameStack* make_frame_stack();
void print_stack_local_frame(void* local_frame);
void* make_local_frame(void* parent, AddressObj* ra, int nslots);
void local_frame_slot_set(void* local_frame, int index, void* value);
void* local_frame_slot_get(void* local_frame, int index);

// operand stack
extern OperandStack* operand_stack;
extern void* operand_pointer;

OperandStack* make_operand_stack();
void* operand_stack_peek();
void operand_stack_add(void* value);
void* operand_stack_pop();
void print_operand_stack(void* operand_pointer);

// code section
CodeSection* make_code_section();

#endif