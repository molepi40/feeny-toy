#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mman.h>
#include "../utils/utils.h"
#include "../compiler/bytecode.h"
#include "./object.h"
#include "./state.h"
#include "./mem.h"
#include "./jit.h"

extern MemManager* mem_manager;
extern ConstantPool* pool;
extern GlobalEnvironment* genv;
extern void* global_slots;
extern FrameStack* frame_stack;
extern void* frame_pointer;
extern void* frame_base_pointer;
extern OperandStack* operand_stack;
extern void* operand_pointer;


extern int call_cfeeny(void*);
void* ADDRESS_OF_IP;
void* BYTECODE_INSTR;

void jit_diver(ConstantPool* pool, GlobalEnvironment* genv, 
                MethodObj* method) {
    CodeSection* call_slot_set_up_code_section = make_code_section();
    void* pc = jit_compile_method(method);
    int running = 1;
    while (running) {
        int rv = call_cfeeny(pc);
        switch (rv) {
            case EXIT_CODE: {
                running = 0; // exit program
                break;
            }

            case GC_CODE: {
                garbage_collector();
                pc = ADDRESS_OF_IP;
                break;
            }

            case PRINTF_CODE: {
                PrintfIns* instr1 = (PrintfIns*)BYTECODE_INSTR;

                // format
                StringObj* fmt = (StringObj*)vector_get(pool, instr1->format);
                DEBUG_INFO(fmt->tag != STRING_OBJ, "invalid type of printf format. it must be int");
                char* p = fmt->value;
                
                // arguments in format
                int arity = instr1->arity;
                Vector* args = make_vector();
                void* arg;
                for (int i = 0; i < arity; ++i) {
                    arg = operand_stack_pop();
                    DEBUG_INFO(GET_TAG(arg) != INT_TAG, "invalid type of printf format argument. it must be int");
                    vector_add(args, arg);
                }

                // print
                while (*p != '\0') {
                    if (*p == '~') {
                        DEBUG_INFO(vector_empty(args), "missing argument of printf format");
                        arg = vector_pop(args);
                        printf("%d", GET_INT(arg));
                    } else {
                        printf("%c", *p);
                    }
                    ++p;
                }
                DEBUG_INFO(!vector_empty(args), "too many arguments for printf format");

                // put null obj in operand stack as ret value
                void* null_obj = make_tagged_null();
                operand_stack_add(null_obj);
                pc = ADDRESS_OF_IP;
                break;                
            }

            case SLOT_CODE: {
                SlotIns* instr1 = (SlotIns*)BYTECODE_INSTR;
                void* tagged_instance_obj = operand_stack_pop();
                StringObj* slot_name = vector_get(pool, instr1->name);
                DEBUG_INFO(slot_name->tag != STRING_OBJ, "invalid type of slot name. it must be string");
                
                // get original instance pointer and check its type
                DEBUG_INFO(GET_TAG(tagged_instance_obj) != HEAP_OBJECT_TAG, "invalid type of receiver for instance var get");
                void* instance_obj = GET_INSTANCE(tagged_instance_obj);
                DEBUG_INFO(OBJ_TAG(instance_obj) != INSTANCE_HEAP_OBJ, "invalid type of receiver for instance var get");
                
                // get class
                ClassObj* class_obj = GET_INSTANCE_CLASS(instance_obj);
                int layer = 0;  
                int var_index = class_obj_get_var(class_obj, slot_name);
                
                // look up slot in parent object
                while (var_index < 0) {
                    tagged_instance_obj = GET_INSTANCE_PARENT(instance_obj);
                    DEBUG_INFO(GET_TAG(tagged_instance_obj) == NULL_TAG, "var slot not in instance");
                    instance_obj = GET_INSTANCE(tagged_instance_obj);
                    class_obj = GET_INSTANCE_CLASS(instance_obj);
                    layer++;
                    var_index = class_obj_get_var(class_obj, slot_name);
                }
                
                // get slot value
                void* slot_value = GET_INSTANCE_SLOT(instance_obj, var_index);                
                operand_stack_add(slot_value);
                
                // update cache
                *((uint64_t*)(ADDRESS_OF_IP - 8)) = (uint64_t)class_obj;
                *((uint64_t*)(ADDRESS_OF_IP - 16)) = (int64_t)layer;
                *((uint64_t*)(ADDRESS_OF_IP - 24)) = (int64_t)var_index;
                
                // trap return address
                pc = ADDRESS_OF_IP;
                break;                
            }

            case SET_SLOT_CODE: {
                SetSlotIns* instr1 = (SetSlotIns*)BYTECODE_INSTR;
                void* value = operand_stack_pop();
                void* tagged_instance_obj = operand_stack_pop();
                StringObj* slot_name = vector_get(pool, instr1->name);
                
                // get original instance pointer and check its type
                DEBUG_INFO(GET_TAG(tagged_instance_obj) != HEAP_OBJECT_TAG, "invalid type of receiver for instance var get");
                void* instance_obj = GET_INSTANCE(tagged_instance_obj);
                DEBUG_INFO(OBJ_TAG(instance_obj) != INSTANCE_HEAP_OBJ, "invalid type of receiver for instance var get");
                
                // get class
                ClassObj* class_obj = GET_INSTANCE_CLASS(instance_obj);
                int layer = 0;  
                int var_index = class_obj_get_var(class_obj, slot_name);
                
                // look up slot in parent object
                while (var_index < 0) {
                    tagged_instance_obj = GET_INSTANCE_PARENT(instance_obj);
                    DEBUG_INFO(GET_TAG(tagged_instance_obj) == NULL_TAG, "var slot not in instance");
                    instance_obj = GET_INSTANCE(tagged_instance_obj);
                    class_obj = GET_INSTANCE_CLASS(instance_obj);
                    layer++;
                    var_index = class_obj_get_var(class_obj, slot_name);
                }
                
                // get slot value
                GET_INSTANCE_SLOT(instance_obj, var_index) = value;              
                operand_stack_add(value);
                
                // update cache
                *((uint64_t*)(ADDRESS_OF_IP - 8)) = (uint64_t)class_obj;
                *((uint64_t*)(ADDRESS_OF_IP - 16)) = (int64_t)layer;
                *((uint64_t*)(ADDRESS_OF_IP - 24)) = (int64_t)var_index;
                
                // trap return address
                pc = ADDRESS_OF_IP;
                break;
            }

            case CALL_SLOT_CODE: {
                CallSlotIns* instr1 = (CallSlotIns*)BYTECODE_INSTR;
                StringObj* method_name = vector_get(pool, instr1->name);

                int arity = instr1->arity;
                void* receiver = *((void**)(operand_pointer - ADDR_WIDTH * arity));

                if (GET_TAG(receiver) == HEAP_OBJECT_TAG) {
                    // object
                    void* pointer = GET_POINTER(receiver);
                    // check tag of the object
                    if (OBJ_TAG(pointer) == INSTANCE_HEAP_OBJ) {
                        // the receiver is an instance of class
                        MethodObj* method_obj = tagged_instance_get_method(receiver, method_name);

                        // compile method body
                        pc = jit_compile_method(method_obj);
                        // make prelude code section for call slot
                        void* prelude = jit_emit_set_up_call_slot(call_slot_set_up_code_section, ADDRESS_OF_IP, \
                                                                  arity, method_obj->nlocals, pc);        
                        // update call site cache
                        // printf("record");
                        *((uint64_t*)(ADDRESS_OF_IP - 16)) = (uint64_t)prelude;
                        *((uint64_t*)(ADDRESS_OF_IP - 8)) = (uint64_t)GET_INSTANCE_CLASS(pointer);
                        pc = prelude;
                        break;
                    } else {
                        DEBUG_INFO(1, "invalid receiver for instance call slot");
                    }            
                } else {
                    DEBUG_INFO(1, "invalid receiver for call slot");
                }
                break;                
            }
        }
    }                                
}