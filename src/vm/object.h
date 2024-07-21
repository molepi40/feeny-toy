#ifndef OBJECT_H
#define OBJECT_H
#include "../utils/utils.h"
#include "../utils/hash.h"

#define TAG_WIDTH 4
#define ADDR_WIDTH 8
#define MIN_OBJ_SIZE 16

#define OBJ_TAG(obj) *((ObjTag*)(obj))
#define GET_SIZE(obj) *((int*)(obj + 4))

// tagged primitive null
#define TAGGED_NULL (void*)0x2
#define TAGGED_ZERO (void*)0x0

// get tag
#define GET_TAG(pointer) ((uint64_t)pointer & 0x7)
#define IS_TAGGED_PRIMITIVE(pointer) (GET_TAG(pointer) == INT_TAG || GET_TAG(pointer) == NULL_TAG)
#define IS_TAGGED_OBJECT(pointer) (GET_TAG(pointer) == 0x1)

// tagged primitive int, use int not uint
#define GET_INT(tagged) (int)((int64_t)tagged >> 3)
#define MAKE_TAGGED_INT(value) (void*)((int64_t)value << 3)

// get pointer from tagged pointer
#define GET_POINTER(tagged) (void*)((uint64_t)tagged & 0xfffffffffffffff8)

// array: ObjTag(4B), size(4B), length(8B), data
#define GET_ARRAY_LENGTH(obj) *((int*)(obj + 8))
#define GET_ARRAY_DATA(obj, idx) *((void**)(obj + MIN_OBJ_SIZE + idx * ADDR_WIDTH))
#define MAKE_TAGGED_ARRAY(obj) (void*)((uint64_t)obj | 0x1)
#define GET_ARRAY(tagged) (void*)((uint64_t)tagged & 0xfffffffffffffff8)

// instance: ObjTag(4B), size(4B), class(8B), parent(8B), slots
#define GET_INSTANCE_CLASS(obj) *((ClassObj**)(obj + 8))
#define GET_INSTANCE_PARENT(obj) *((void**)(obj + 16))
#define GET_INSTANCE_SLOT(obj, idx) *((void**)(obj + 24 + idx * ADDR_WIDTH))
#define MAKE_TAGGED_INSTANCE(obj) (void*)((uint64_t)obj | 0x1)
#define GET_INSTANCE(tagged) (void*)((uint64_t)tagged & 0xfffffffffffffff8)

/**
 * There is some kind of difference between object declared in Constant: and the exact instance
 * in the execution of program.
 * 
 * For part dealing Constant, all the reference index should be replaced with the actual object in 
 * constant pool.
 */

typedef enum {
    // dynamic heap objects
    INT_TAG,
    HEAP_OBJECT_TAG,
    NULL_TAG,

    ARRAY_HEAP_OBJ,
    INSTANCE_HEAP_OBJ,
    B_HEART,

    // constant objects
    NULL_OBJ,
    INT_OBJ,
    STRING_OBJ,
    SLOT_OBJ,
    METHOD_OBJ,
    CLASS_OBJ,

    LABEL_OBJ,
    ADDRESS_OBJ,
} ObjTag;


/**
 * All these defined structs are constant pool objects.
 */

typedef struct {
    ObjTag tag;
} Obj;

typedef struct {
    ObjTag tag;
} NullObj;

typedef struct {
    ObjTag tag;
    int value;
} IntObj;

typedef struct {
    ObjTag tag;
    int length;
    char* value;
} StringObj;

typedef struct {
    ObjTag tag;
    StringObj* name;
} SlotObj;

typedef struct {
    ObjTag tag;
    StringObj* name;
    int nargs;
    int nlocals;
    Vector* code;
    void* assembly;
} MethodObj;

typedef struct {
    ObjTag tag;
    int nslots;
    HashTable* var_ht;
    HashTable* method_ht;
} ClassObj;

typedef struct {
    ObjTag tag;
    MethodObj* method;
    int addr;
    void* assembly_addr;
    Vector* backfill_list;
} LabelObj;

typedef struct {
    ObjTag tag;
    MethodObj* method;
    int addr;
} AddressObj;


/**
 * Functions for tagged primitives
 */

// ObjTag(4), size(4), padding(8)
void* make_tagged_null();

// ObjTag(4), size(4), int(4), padding(4)
void* make_tagged_int(int value);
void* calc(void* tagged_x, void* tagged_y, StringObj* op_obj);

/**
 * Functions for dynamic heap objects.
 */

// ObjTag(4), size(4), length(8), ADDR_WIDTH * length
void* make_tagged_array(void* tagged_length, void* tagged_init);
void* tagged_array_get(void* tagged_array, void* tagged_index);
void tagged_array_set(void* tagged_array, void* tagged_index, void* tagged_value);
void* tagged_array_length(void* tagged_array);

void print_tagged_array(void* tagged_array);
void print_array(void* array);

// ObjTag(4), size(4), class(8), parent(8), slot_value(8) * nslots
void* make_tagged_instance(ClassObj* class, void* tagged_parent, Vector* vars);
void* tagged_instance_get_var(void* tagged_instance_obj, StringObj* var_name);
void tagged_instance_set_var(void* tagged_instance_obj, StringObj* var_name, void* tagged_value);
MethodObj* tagged_instance_get_method(void* tagged_instance_obj, StringObj* method_name);

/**
 * Functions for constant pool objects.
 */

NullObj* make_null_obj();

IntObj* make_int_obj(int value);

StringObj* make_string_obj(char* string);

SlotObj* make_slot_obj(StringObj* name);

MethodObj* make_method_obj(StringObj* name, int nargs, int nlocals, Vector* code);

ClassObj* make_class_obj();
int class_obj_get_var(ClassObj* class_obj, StringObj* var_name);
MethodObj* class_obj_get_method(ClassObj* class_obj, StringObj* method_name);
void class_obj_add_var(ClassObj* class_obj, SlotObj* slot_obj);
void class_obj_add_method(ClassObj* class_obj, MethodObj* method_obj);

LabelObj* make_label_obj(MethodObj* method_obj, int addr);
void label_obj_set_assembly(LabelObj* label_obj, void* assembly_addr);
void label_obj_get_assembly(LabelObj* label_obj);
void label_obj_add_backfill(LabelObj* label_obj, void* backfill_addr);
void label_obj_backfill(LabelObj* label_obj, void* assembly_addr);

AddressObj* make_address_obj(MethodObj* method_obj, int addr);

#endif