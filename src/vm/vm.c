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
#include "./vm.h"


MemManager* mem_manager;
// heap pointer
void* heap_pointer;
void* top_of_heap;

// constant pool
ConstantPool* pool;

// global symbol_table
GlobalEnvironment* genv;
// start addr of global slots
void* global_slots;

// frame stack
FrameStack* frame_stack;
// local frame pointer
void* frame_pointer;
// frame base pointer
void* frame_base_pointer;

// operand stack
OperandStack* operand_stack;
// stack pointer
void* operand_pointer;


void interpret_bc (Program* p, int jit_flag) {
  printf("Interpreting Bytecode Program:\n");

  // create memory manager
  mem_manager = (MemManager*)malloc(sizeof(MemManager));
  initilize_mem_manager();

  // create constant pool
  pool = make_vector();

  // add values into constant pool and collect method slots
  Vector* method_slots =  make_vector();
  add_constant_pool(pool, method_slots, p->values); 
  
  // create global environment and add global slots
  genv = make_global_env();
  int global_nslots = add_global_slots(pool, genv, p->slots);
  global_slots = make_global_slots(global_nslots);

  // add method instructions, labels
  for (int i = 0; i < method_slots->size; ++i) {
    MethodObj* method_obj = (MethodObj*)vector_get(method_slots, i);
    add_method_label(pool, genv, method_obj);
  }
  delete_vector(method_slots);

  // get entry
  MethodObj* entry = vector_get(pool, p->entry);
  DEBUG_INFO(entry->tag != METHOD_OBJ, "invalid type of entry. it must be method");

  // create local frame and operand stack for entry
  frame_stack = make_frame_stack();
  frame_base_pointer = frame_pointer = frame_stack->stack;
  void* local_frame = make_local_frame(NULL, NULL, entry->nlocals);

  // create operand stack
  operand_stack = make_operand_stack();
  operand_pointer = operand_stack->stack;

  // execution on
  if (jit_flag){
    jit_diver(pool, genv, entry);
  } else {
    exec_instr(pool, genv, global_slots, local_frame, entry);
  } 
}


// add global slots to global table and return number of slot object
int add_global_slots(ConstantPool* pool, GlobalEnvironment* genv, Vector* slots) {
  int cnt = 0;

  for (int i = 0; i < slots->size; ++i) {
    int slot = (int)vector_get(slots, i);
    Obj* obj = vector_get(pool, slot);

    switch (obj->tag) {
      case SLOT_OBJ: {
        SlotObj* obj1 = (SlotObj*)obj;
        global_env_add_var(genv, obj1, cnt);
        cnt++;
        break;       
      }
      case METHOD_OBJ: {
        MethodObj* obj1 = (MethodObj*)obj;
        global_env_add_method(genv, obj1);
        break;
      }
      default: {
        DEBUG_INFO(0, "invalid type of global slot.");
        break;
      }
    }
  }
  return cnt;
}

void add_constant_pool(ConstantPool* pool, Vector* method_slots, Vector* values) {
  Value* value;
  for (int i = 0; i < values->size; ++i) {
    value = vector_get(values, i);
    switch (value->tag) {

      case NULL_VAL: {
        NullObj* null_obj = make_null_obj();
        vector_set(pool, i, null_obj);
        break;
      }

      case INT_VAL: {
        IntValue* value1 = (IntValue*)value;
        IntObj* int_obj = make_int_obj(value1->value);
        vector_set(pool, i, int_obj);
        break;
      }

      case STRING_VAL: {
        StringValue* value1 = (StringValue*)value;
        StringObj* string_obj = make_string_obj(value1->value);
        vector_set(pool, i, string_obj);
        break;
      }

      case METHOD_VAL: {
        MethodValue* value1 = (MethodValue*)value;
        StringObj* name = vector_get(pool, value1->name);
        DEBUG_INFO(name->tag != STRING_OBJ, "invalid type of method name. it must be string");
        MethodObj* method_obj = make_method_obj(name, value1->nargs, value1->nlocals, value1->code);
        vector_set(pool, i, method_obj);
        vector_add(method_slots, method_obj); // collect all methods to get all labels in method latter
        break;
      }

      case SLOT_VAL: {
        SlotValue* value1 = (SlotValue*)value;
        StringObj* name = vector_get(pool, value1->name);
        DEBUG_INFO(name->tag != STRING_OBJ, "invalid type of slot name. it must be string");
        SlotObj* slot_obj = make_slot_obj(name);
        vector_set(pool, i, slot_obj);
        break;
      }

      case CLASS_VAL: {
        ClassValue* value1 = (ClassValue*)value;
        Vector* slots = value1->slots;
        ClassObj* class_obj = make_class_obj();

        int slot_index;
        Obj* slot_obj;
        // divide slots to variable and method
        for (int i = 0; i < slots->size; ++i) {
          slot_index = (int)vector_get(slots, i);
          slot_obj = (Obj*)vector_get(pool, slot_index);

          switch (slot_obj->tag) {
            case SLOT_OBJ: {
              class_obj_add_var(class_obj, slot_obj);
              break;
            }
            case METHOD_OBJ: {
              class_obj_add_method(class_obj, slot_obj);
              break;
            }
            default: {
              DEBUG_INFO(1, "invalid type of class slot");            
            }
          }
        }
        vector_set(pool, i, class_obj);
        break;
      }
    }
  }
}

void add_method_label(ConstantPool* pool, GlobalEnvironment* genv, MethodObj* method) {
  ByteIns* instr;
  Vector* code = method->code;

  for (int i = 0; i < code->size; ++i) {
    instr = (ByteIns*)vector_get(code, i);

    // if the instr is label, record the label
    if (instr->tag == LABEL_OP) {
      LabelIns* instr1 = (LabelIns*)instr;
      StringObj* label_name = vector_get(pool, instr1->name);
      DEBUG_INFO(label_name->tag != STRING_OBJ, "invalid type of label name. it must be string");
      LabelObj* label_obj = make_label_obj(method, i);
      global_env_add_label(genv, label_name, label_obj);
    }
  }
}


void exec_instr(ConstantPool* pool, GlobalEnvironment* genv, 
                void* global_slots, void* local_frame, 
                MethodObj* method) {
  
  Vector* code = method->code;
  int instr_num = vector_size(code);
  int pc = 0;

  while (pc < instr_num) {
    ByteIns* instr = (ByteIns*)vector_get(code, pc);
    // print_ins(instr);
    // printf("\n");

    switch (instr->tag) {

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
        operand_stack_add(literal);
        break;
      }

      case ARRAY_OP: {
        void* init = operand_stack_pop();
        void* length = operand_stack_pop();
        void* array = make_tagged_array(length, init);
        operand_stack_add(array);
        break;
      }

      case PRINTF_OP: {
        PrintfIns* instr1 = (PrintfIns*)instr;

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
        break;
      }

      case SET_LOCAL_OP: {
        SetLocalIns* instr1 = (SetLocalIns*)instr;
        void* value = operand_stack_peek();
        local_frame_slot_set(local_frame, instr1->idx, value);
        break;
      }

      case GET_LOCAL_OP: {
        GetLocalIns* instr1 = (GetLocalIns*)instr;
        void* value = local_frame_slot_get(local_frame, instr1->idx);
        operand_stack_add(value);
        break;
      }

      case SET_GLOBAL_OP: {
        SetGlobalIns* instr1 = (SetGlobalIns*)instr;
        StringObj* name = vector_get(pool, instr1->name);
        DEBUG_INFO(name->tag != STRING_OBJ, "invalid type of variable name. it must be string");
        int index = global_env_get_var_index(genv, name);
        void* value = operand_stack_peek();
        global_slots_set(global_slots, index, value);
        break;
      }

      case GET_GLOBAL_OP: {
        GetGlobalIns* instr1 = (GetGlobalIns*)instr;
        StringObj* name = vector_get(pool, instr1->name);
        DEBUG_INFO(name->tag != STRING_OBJ, "invalid type of variable name. it must be string");
        int index = global_env_get_var_index(genv, name);
        void* value = global_slots_get(global_slots, index);
        operand_stack_add(value);
        break;
      }

      case DROP_OP: {
        operand_stack_pop();
        break;
      }

      case OBJECT_OP: {
        ObjectIns* instr1 = (ObjectIns*)instr;
        ClassObj* class_obj = (ClassObj*)vector_get(pool, instr1->class);

        // pop n initial variable values
        int nslots = class_obj->nslots;
        Vector* slots = make_vector();
        for (int i = 0; i < nslots; ++i) {
          void* slot = operand_stack_pop();
          vector_add(slots, slot);
        }

        // get parent instance
        void* parent = operand_stack_pop();

        // create a new instance and initialize the variables
        void* instance_obj = make_tagged_instance(class_obj, parent, slots);

        operand_stack_add(instance_obj);
        break;
      }

      case SLOT_OP: {
        SlotIns* instr1 = (SlotIns*)instr;
        void* instance_obj = operand_stack_pop();
        StringObj* slot_name = vector_get(pool, instr1->name);
        DEBUG_INFO(slot_name->tag != STRING_OBJ, "invalid type of slot name. it must be string");
        void* slot_value = tagged_instance_get_var(instance_obj, slot_name);
        operand_stack_add(slot_value);
        break;
      }

      case SET_SLOT_OP: {
        SetSlotIns* instr1 = (SetSlotIns*)instr;
        void* value = operand_stack_pop();
        void* instance_obj = operand_stack_pop();
        StringObj* slot_name = vector_get(pool, instr1->name);
        tagged_instance_set_var(instance_obj, slot_name, value);
        operand_stack_add(value);
        break;
      }

      case CALL_SLOT_OP: {
        CallSlotIns* instr1 = (CallSlotIns*)instr;
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
              break;
            }

            case INSTANCE_HEAP_OBJ: {
              // the receiver is an instance of class
              MethodObj* method_obj = tagged_instance_get_method(receiver, method_name);

              nargs++; // the receiver will be pushed into arg slots
              DEBUG_INFO(method_obj->nargs != nargs, "number of arguments not equal for method and call slot");

              // make ra and new local frame
              AddressObj* ra = make_address_obj(method, pc);
              void* frame = make_local_frame(local_frame, ra, nargs + method_obj->nlocals);

              // argument
              local_frame_slot_set(frame, 0, receiver);
              for (int i = 1; i < nargs; ++i) {
                local_frame_slot_set(frame, i, vector_pop(args));
              }

              // switch context
              local_frame = frame;
              method = method_obj;
              code = method->code;
              instr_num = code->size;
              pc = 0;
              continue;
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

      case LABEL_OP: {
        // do nothing for labels have been added to global table in preprocess
        break;
      }

      case BRANCH_OP: {
        BranchIns* instr1 = (BranchIns*)instr;
        void* value = operand_stack_pop();

        // switch in method, just set new pc
        if (GET_TAG(value) != NULL_TAG) {
          StringObj* label_name = vector_get(pool, instr1->name);
          DEBUG_INFO(label_name->tag != STRING_OBJ, "invalid type of label name");
          LabelObj* label_obj  = global_env_get_label(genv, label_name);
          DEBUG_INFO(label_obj->tag != LABEL_OBJ, "invalid type of label");
          pc = label_obj->addr;
        }
        break;
      }

      case GOTO_OP: {
        GotoIns* instr1 = (GotoIns*)instr;
        StringObj* label_name = vector_get(pool, instr1->name);
        DEBUG_INFO(label_name->tag != STRING_OBJ, "invalid type of label name");
        LabelObj* label_obj = global_env_get_label(genv, label_name);
        DEBUG_INFO(label_obj->tag != LABEL_OBJ, "invalid type of label");
        pc = label_obj->addr;
        break;
      }

      case RETURN_OP: {
        // direclty switch the method in current thread
        void* parent_frame = GET_FRAME_PARENT(local_frame);
        if (parent_frame == NULL) { // entry function
          break;
        }

        // switch context
        AddressObj* ra = GET_FRAME_RA(local_frame);
        method = ra->method;
        code = method->code;
        instr_num = code->size;        
        pc = ra->addr;
        memset(local_frame, 0, 24 + GET_FRAME_NSLOTS(local_frame) * ADDR_WIDTH); // clear discarded frame
        // restore frame
        frame_pointer = local_frame;
        frame_base_pointer =  local_frame = parent_frame;
        break;
      }

      case CALL_OP: {
        CallIns* instr1 = (CallIns*)instr;

        StringObj* method_name = vector_get(pool, instr1->name);
        DEBUG_INFO(method_name->tag != STRING_OBJ, "invalid type of method name. it must be string");
        MethodObj* method_obj = global_env_get_method(genv, method_name); 

        Vector* args = make_vector();
        int nargs = instr1->arity;
        assert(nargs == method_obj->nargs);
        for (int i = 0; i < nargs; ++i) {
          vector_add(args, operand_stack_pop());
        }

        AddressObj* ra = make_address_obj(method, pc);
        void* frame = make_local_frame(local_frame, ra, nargs + method_obj->nlocals);
        for (int i = 0; i < nargs; ++i) {
          void* arg = vector_pop(args);
          local_frame_slot_set(frame, i, arg);
        }
        delete_vector(args);
        
        // switch context
        local_frame = frame;
        method = method_obj;
        code = method->code;
        instr_num = code->size;
        pc = 0;
        continue;
      }
    }
    pc++;    
  }
}