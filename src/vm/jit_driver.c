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
extern CodeSection* code_section;
extern void* code_pointer;

extern int call_cfeeny(void*);
void* ADDRESS_OF_IP;
void* BYTECODE_INSTR;

void jit_diver(ConstantPool* pool, GlobalEnvironment* genv, 
                MethodObj* method) {
    
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
                void* instance_obj = operand_stack_pop();
                StringObj* slot_name = vector_get(pool, instr1->name);
                DEBUG_INFO(slot_name->tag != STRING_OBJ, "invalid type of slot name. it must be string");
                void* slot_value = tagged_instance_get_var(instance_obj, slot_name);
                operand_stack_add(slot_value);
                pc = ADDRESS_OF_IP;
                break;                
            }

            case SET_SLOT_CODE: {
                SetSlotIns* instr1 = (SetSlotIns*)BYTECODE_INSTR;
                void* value = operand_stack_pop();
                void* instance_obj = operand_stack_pop();
                StringObj* slot_name = vector_get(pool, instr1->name);
                tagged_instance_set_var(instance_obj, slot_name, value);
                operand_stack_add(value);
                pc = ADDRESS_OF_IP;
                break;
            }

            case CALL_SLOT_CODE: {
                CallSlotIns* instr1 = (CallSlotIns*)BYTECODE_INSTR;
                StringObj* method_name = vector_get(pool, instr1->name);

                // get args
                int nargs = instr1->arity - 1; // exact arguments
                Vector* args = make_vector();
                for (int i = 0; i < nargs; ++i) {
                    void* arg = operand_stack_pop();
                    vector_add(args, arg);
                }

                // receiver for call slot
                void* receiver = operand_stack_pop();

                if (GET_TAG(receiver) == INT_TAG) {
                    // int
                    DEBUG_INFO(nargs != 1, "invalid nargs for int operation. it must be 1");
                    void* outcome = calc(receiver, vector_pop(args), method_name);
                    operand_stack_add(outcome);
                    pc = ADDRESS_OF_IP;

                } else if (GET_TAG(receiver) == HEAP_OBJECT_TAG) {
                    // object
                    void* pointer = GET_POINTER(receiver);
                    // check tag of the object
                    switch (OBJ_TAG(pointer)) {
                        // array
                        case ARRAY_HEAP_OBJ: {
                            // array[index]
                            if (strcmp(method_name->value, "get") == 0) {
                                DEBUG_INFO(nargs != 1, "invalid nargs for array get. it must be 1");
                                void* index = vector_pop(args);
                                void* value = tagged_array_get(receiver, index);
                                operand_stack_add(value);
                            // array[index] = value
                            } else if (strcmp(method_name->value, "set") == 0) {
                                DEBUG_INFO(nargs != 2, "invalid nargs for array set. it must be 2");
                                void* index = vector_pop(args);
                                void* value = vector_pop(args);
                                tagged_array_set(receiver, index, value);
                                void* null_obj = make_tagged_null();
                                operand_stack_add(null_obj);
                            // array.length()
                            } else if (strcmp(method_name->value, "length") == 0) {
                                DEBUG_INFO(nargs != 0, "invalid nargs for array length. it must be 0");
                                void* length = tagged_array_length(receiver);
                                operand_stack_add(length);
                            } else {
                                DEBUG_INFO(1, "invalid method for receiver as array");
                            }
                            pc = ADDRESS_OF_IP;
                            break;
                        }

                        case INSTANCE_HEAP_OBJ: {
                            // the receiver is an instance of class
                            MethodObj* method_obj = tagged_instance_get_method(receiver, method_name);

                            nargs++; // the receiver will be pushed into arg slots
                            DEBUG_INFO(method_obj->nargs != nargs, "number of arguments not equal for method and call slot");

                            // make new local frame
                            void* frame = make_local_frame(frame_base_pointer, ADDRESS_OF_IP, nargs + method_obj->nlocals);

                            // argument
                            local_frame_slot_set(frame, 0, receiver);
                            for (int i = 1; i < nargs; ++i) {
                                local_frame_slot_set(frame, i, vector_pop(args));
                            }

                            // switch context
                            pc = jit_compile_method(method_obj);
                            break;
                        }

                        default: {
                            DEBUG_INFO(1, "invalid receiver for instance call slot");
                        }            
                    }
                } else {
                    DEBUG_INFO(1, "invalid receiver for call slot");
                }
                delete_vector(args);
                break;                
            }
        }
    }                                
}