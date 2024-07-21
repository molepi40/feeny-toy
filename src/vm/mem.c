#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "./object.h"
#include "./state.h"
#include "./mem.h"


// heap size = 4MB = 2097152B
#define HEAP_SIZE 4 * 1024 * 1024
#define GET_FORWARD(obj) *((void**)(obj + 8))

extern MemManager* mem_manager;
extern void* heap_pointer;
extern void* top_of_heap;
extern GlobalEnvironment* genv;
extern void* global_slots;
extern OperandStack* operand_stack;
extern void* operand_pointer;
extern FrameStack* frame_stack;
extern void* frame_base_pointer;
extern void* frame_pointer;


void initilize_mem_manager() {
    mem_manager = (MemManager*)malloc(sizeof(MemManager));
    void* allocated_heap = malloc(HEAP_SIZE);
    mem_manager->to_space  = heap_pointer = allocated_heap;
    mem_manager->from_space = top_of_heap = allocated_heap + HEAP_SIZE / 2;
}

void flip_semispace() {
    void* temp = mem_manager->to_space;
    mem_manager->to_space = mem_manager->from_space;
    mem_manager->from_space = temp;

    heap_pointer = mem_manager->to_space;
    top_of_heap = heap_pointer + HEAP_SIZE / 2;
}

void* halloc(int nbytes) {
    if (nbytes < 0) {
        DEBUG_INFO(1, "invalid memory request. size of memory must >= 0");
    } else if (nbytes == 0) {
        return NULL;
    }

    if (heap_pointer + nbytes > top_of_heap) {
        garbage_collector();
    }
    void* allocated = heap_pointer;
    heap_pointer += nbytes;
    return allocated;
}

void* garbage_collector() {
    // printf("gargabe collect\n");
    flip_semispace();
    // print_root_set();
    // exit(1);
    scan_root_set();
    // printf("test\n");
    
    void* p = mem_manager->to_space;
    void* field_value;
    // printf("%x %x\n", p, top_of_heap);
    while (p < heap_pointer) {
        switch (OBJ_TAG(p)) {
            case ARRAY_HEAP_OBJ: {
                int length = GET_ARRAY_LENGTH(p);
                for (int i = 0; i < length; ++i) {
                    field_value = GET_ARRAY_DATA(p, i);
                    // if (OBJ_TAG(field_value) != 1 && OBJ_TAG(field_value) != 4){
                    //     printf("%d\n", OBJ_TAG(field_value));
                    //     printf("array length: %d\n", GET_ARRAY_LENGTH(field_value));
                    //     exit(1);
                    // }        
                    GET_ARRAY_DATA(p, i) = get_post_gc_ptr(field_value);
                }
                break;
            }

            case INSTANCE_HEAP_OBJ: {
                int nslots = ((ClassObj*)GET_INSTANCE_CLASS(p))->nslots;
                for (int i = 0; i < nslots; ++i) {
                    field_value = GET_INSTANCE_SLOT(p, i);
                    GET_INSTANCE_SLOT(p, i) = get_post_gc_ptr(field_value);
                }
                break;
            }
        }
        p += GET_SIZE(p);
    }
}

void scan_root_set() {
    void* var_value;
    void* forward;

    // global variabls
    int global_nslots = GET_ARRAY_LENGTH(global_slots);
    for (int i = 0; i < global_nslots; ++i) {
        var_value = GET_ARRAY_DATA(global_slots, i);
        GET_ARRAY_DATA(global_slots, i) = get_post_gc_ptr(var_value);
    }  

    // local frame
    int nslots;
    void* frame = frame_base_pointer;
    while (frame) {
        nslots = GET_FRAME_NSLOTS(frame);
        for (int i = 0; i < nslots; ++i) {
            var_value = GET_FRAME_SLOT(frame, i);
            GET_FRAME_SLOT(frame, i) = get_post_gc_ptr(var_value);
        }
        frame = GET_FRAME_PARENT(frame);
    }

    // operand stack
    void* p = operand_stack->stack;
    while (p < operand_pointer) {
        var_value = GET_STACK_VALUE(p);
        GET_STACK_VALUE(p) = get_post_gc_ptr(var_value);
        p += ADDR_WIDTH;
    }
}

void print_root_set() {
    printf("global variables:\n");
    print_global_variables(global_slots);
    printf("\n");

    printf("stack of local frames:\n");
    print_stack_local_frame(frame_base_pointer);
    printf("\n");

    printf("operand stack:\n");
    print_operand_stack(operand_pointer);
}


void* get_post_gc_ptr(void* tagged_obj) {
    if (tagged_obj == NULL) {
        return NULL;
    }
    if (IS_TAGGED_PRIMITIVE(tagged_obj)) {
        return tagged_obj;
    }
    void* forward;
    void* obj = GET_INSTANCE(tagged_obj);
    if (OBJ_TAG(obj) == B_HEART) {
        forward = GET_FORWARD(obj);
    } else {
        forward = heap_pointer;
        int obj_size = GET_SIZE(obj);
        if (forward + obj_size > top_of_heap) {
            printf("memory run out\n");
            exit(1);
        }
        heap_pointer += obj_size;
        // shallow copy, the field point to original memory
        memcpy(forward, obj, obj_size); 
        OBJ_TAG(obj) = B_HEART;
        GET_FORWARD(obj) = forward;
    }
    return MAKE_TAGGED_INSTANCE(forward);
}