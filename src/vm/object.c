#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../utils/utils.h"
#include "../utils/hash.h"
#include "../compiler/bytecode.h"
#include "./object.h"
#include "./mem.h"


/**
 * Tagged primitives, including null and int. 
 */

void* make_tagged_null() {
    return TAGGED_NULL;
}

void* make_tagged_int(int value) {
    return MAKE_TAGGED_INT(value);
}



void* add(void* tagged_x, void* tagged_y) {
    return (void*)((int64_t)tagged_x + (int64_t)tagged_y);
}

void* sub(void* tagged_x, void* tagged_y) {
    return (void*)((int64_t)tagged_x - (int64_t)tagged_y);
}

void* mul(void* tagged_x, void* tagged_y) {
    return (void*)(((int64_t)tagged_x * (int64_t)tagged_y) >> 3);
}

void* divi(void* tagged_x, void* tagged_y) {
    return (void*)(((int64_t)tagged_x / (int64_t)tagged_y) << 3);
}

void* mod(void* tagged_x, void* tagged_y) {
    return (void*)((int64_t)tagged_x % (int64_t)tagged_y);
}

void* le(void* tagged_x, void* tagged_y) {
    if ((int64_t)tagged_x <= (int64_t)tagged_y) {
        return TAGGED_ZERO;
    }
    return TAGGED_NULL;
}

void* ge(void* tagged_x, void* tagged_y) {
    if ((int64_t)tagged_x >= (int64_t)tagged_y) {
        return TAGGED_ZERO;
    }
    return TAGGED_NULL;
}

void* lt(void* tagged_x, void* tagged_y) {
    if ((int64_t)tagged_x < (int64_t)tagged_y) {
        return TAGGED_ZERO;
    }
    return TAGGED_NULL;
}

void* gt(void* tagged_x, void* tagged_y) {
    if ((int64_t)tagged_x > (int64_t)tagged_y) {
        return TAGGED_ZERO;
    }
    return TAGGED_NULL;
}

void* eq(void* tagged_x, void* tagged_y) {
    if ((int64_t)tagged_x == (int64_t)tagged_y) {
        return TAGGED_ZERO;
    }
    return TAGGED_NULL;
}

void* calc(void* tagged_x, void* tagged_y, StringObj* op_obj) {
    DEBUG_INFO(GET_TAG(tagged_x) != INT_TAG, "invalid type of calc operand. it must be int");
    DEBUG_INFO(GET_TAG(tagged_y) != INT_TAG, "invalid type of calc operand. it must be int");

    char* op = op_obj->value;
    void* tagged_ret;
    if (strcmp(op, "add") == 0) {
        tagged_ret =  add(tagged_x, tagged_y);
    } else if (strcmp(op, "sub") == 0) {
        tagged_ret = sub(tagged_x, tagged_y);
    } else if (strcmp(op, "mul") == 0) {
        tagged_ret = mul(tagged_x, tagged_y);
    } else if (strcmp(op, "div") == 0) {
        tagged_ret = divi(tagged_x, tagged_y);
    } else if (strcmp(op, "mod") == 0) {
        tagged_ret = mod(tagged_x, tagged_y);
    } else if (strcmp(op, "lt") == 0) {
        tagged_ret = lt(tagged_x, tagged_y);
    } else if (strcmp(op, "gt") == 0) {
        tagged_ret = gt(tagged_x, tagged_y);
    } else if (strcmp(op, "le") == 0) {
        tagged_ret = le(tagged_x, tagged_y);
    } else if (strcmp(op, "ge") == 0) {
        tagged_ret = ge(tagged_x, tagged_y);
    } else if (strcmp(op, "eq") == 0) {
        tagged_ret = eq(tagged_x, tagged_y);
    } else {
        DEBUG_INFO(1, "invalid operator for calc.");
    }
    return tagged_ret;
}

/**
 * Dynamic heap objects use garbage collector to manage memory on heap.
 * Each object has at least 2 words = 16 bytes = 128 bits.
 */

void* make_tagged_array(void* tagged_length, void* tagged_init) {
    // get length and check if it is in range
    DEBUG_INFO(GET_TAG(tagged_length) != INT_TAG, "invalid type of length. it must be int");
    int elems_length = GET_INT(tagged_length);
    DEBUG_INFO(elems_length < 0, "length of array must greater or equal to 0");

    int size = MIN_OBJ_SIZE + elems_length * ADDR_WIDTH; // multiple of 8

    void* array_obj = halloc(size);
    memset(array_obj, 0, size);

    OBJ_TAG(array_obj) = ARRAY_HEAP_OBJ;
    GET_SIZE(array_obj) = size;
    GET_ARRAY_LENGTH(array_obj) = elems_length;
    
    void* p = array_obj + MIN_OBJ_SIZE; // data start point
    for (int i = 0; i < elems_length; ++i) {
        *(void**)p = tagged_init;
        p += ADDR_WIDTH;
    }
    
    return MAKE_TAGGED_ARRAY(array_obj);
}

void* tagged_array_get(void* tagged_array, void* tagged_index) {
    // get original array pointer and check its type
    DEBUG_INFO(GET_TAG(tagged_array) != HEAP_OBJECT_TAG, "invalid type of receiver for array set");
    void* array = GET_ARRAY(tagged_array);
    DEBUG_INFO(OBJ_TAG(array) != ARRAY_HEAP_OBJ, "invalid type of receiver for array set")

    // extract int value from tagged int
    DEBUG_INFO(GET_TAG(tagged_index) != INT_TAG, "invalid type of index. it must be int");
    int idx = GET_INT(tagged_index);
    int array_length = GET_ARRAY_LENGTH(array);
    DEBUG_INFO(idx < 0 || idx >= array_length, "array index out of range.");

    void* tagged_value = GET_ARRAY_DATA(array, idx);

    return tagged_value;
}

void tagged_array_set(void* tagged_array, void* tagged_index, void* tagged_value) {
    // get original array pointer and check its type
    DEBUG_INFO(GET_TAG(tagged_array) != HEAP_OBJECT_TAG, "invalid type of receiver for array set");
    void* array = GET_ARRAY(tagged_array);
    DEBUG_INFO(OBJ_TAG(array) != ARRAY_HEAP_OBJ, "invalid type of receiver for array set")

    // extract int value from tagged int
    DEBUG_INFO(GET_TAG(tagged_index) != INT_TAG, "invalid type of index. it must be int");
    int idx = GET_INT(tagged_index);
    int array_length = GET_ARRAY_LENGTH(array);
    DEBUG_INFO(idx < 0 || idx >= array_length, "array index out of range.");

    GET_ARRAY_DATA(array, idx) = tagged_value;
}

void* tagged_array_length(void* tagged_array) {
    // get original array pointer and check its type
    DEBUG_INFO(GET_TAG(tagged_array) != HEAP_OBJECT_TAG, "invalid type of receiver for array length");
    void* array = GET_ARRAY(tagged_array);
    DEBUG_INFO(OBJ_TAG(array) != ARRAY_HEAP_OBJ, "invalid type of receiver for array length")

    int array_length = GET_ARRAY_LENGTH(array);
    return make_tagged_int(array_length);
}

void* make_tagged_instance(ClassObj* class, void* tagged_parent, Vector* vars) {
    // ger original parent pointer and check its type 
    void* parent;
    if (GET_TAG(tagged_parent) == HEAP_OBJECT_TAG) {
        parent = GET_INSTANCE(tagged_parent);
        DEBUG_INFO(OBJ_TAG(parent) != INSTANCE_HEAP_OBJ, "invalid type of parent for object");
    } else {
        DEBUG_INFO(GET_TAG(tagged_parent) != NULL_TAG, "invalid type of parent for object")
    }

    int nslots = class->nslots;
    int size = 24 + ADDR_WIDTH * nslots;

    void* instance_obj = halloc(size);
    memset(instance_obj, 0, size);
    OBJ_TAG(instance_obj) = INSTANCE_HEAP_OBJ;
    GET_SIZE(instance_obj) = size;
    GET_INSTANCE_CLASS(instance_obj) = class;
    GET_INSTANCE_PARENT(instance_obj) = tagged_parent;

    int nvars = vector_size(vars);
    for (int i = 0; i < nvars; ++i) {
        GET_INSTANCE_SLOT(instance_obj, i) = vector_pop(vars);
    }

    return MAKE_TAGGED_INSTANCE(instance_obj);
}

void* tagged_instance_get_var(void* tagged_instance_obj, StringObj* var_name) {
    // get original instance pointer and check its type
    DEBUG_INFO(GET_TAG(tagged_instance_obj) != HEAP_OBJECT_TAG, "invalid type of receiver for instance var get");
    void* instance_obj = GET_INSTANCE(tagged_instance_obj);
    DEBUG_INFO(OBJ_TAG(instance_obj) != INSTANCE_HEAP_OBJ, "invalid type of receiver for instance var get");

    ClassObj* class_obj = GET_INSTANCE_CLASS(instance_obj);
    int var_index = class_obj_get_var(class_obj, var_name);

    while (var_index < 0) {
        tagged_instance_obj = GET_INSTANCE_PARENT(instance_obj);
        DEBUG_INFO(GET_TAG(tagged_instance_obj) == NULL_TAG, "var slot not in instance");
        instance_obj = GET_INSTANCE(tagged_instance_obj);
        class_obj = GET_INSTANCE_CLASS(instance_obj);
        var_index = class_obj_get_var(class_obj, var_name);
    }

    void* tagged_value = GET_INSTANCE_SLOT(instance_obj, var_index);

    return tagged_value;
}

void tagged_instance_set_var(void* tagged_instance_obj, StringObj* var_name, void* tagged_value) {
    // get original instance pointer and check its type
    DEBUG_INFO(GET_TAG(tagged_instance_obj) != HEAP_OBJECT_TAG, "invalid type of receiver for instance var value set");
    void* instance_obj = GET_INSTANCE(tagged_instance_obj);
    DEBUG_INFO(OBJ_TAG(instance_obj) != INSTANCE_HEAP_OBJ, "invalid type of receiver for instance var value set");

    ClassObj* class_obj = GET_INSTANCE_CLASS(instance_obj);
    int var_index = class_obj_get_var(class_obj, var_name);

    while (var_index < 0) {
        tagged_instance_obj = GET_INSTANCE_PARENT(instance_obj);
        DEBUG_INFO(GET_TAG(tagged_instance_obj) == NULL_TAG, "var slot not in instance");
        instance_obj = GET_INSTANCE(tagged_instance_obj);
        class_obj = GET_INSTANCE_CLASS(instance_obj);
        var_index = class_obj_get_var(class_obj, var_name);
    }

    GET_INSTANCE_SLOT(instance_obj, var_index) = tagged_value;
}

MethodObj* tagged_instance_get_method(void* tagged_instance_obj, StringObj* method_name) {
    // get original instance pointer and check its type
    DEBUG_INFO(GET_TAG(tagged_instance_obj) != HEAP_OBJECT_TAG, "invalid type of receiver for instance var value set");
    void* instance_obj = GET_INSTANCE(tagged_instance_obj);
    DEBUG_INFO(OBJ_TAG(instance_obj) != INSTANCE_HEAP_OBJ, "invalid type of receiver for instance var value set");

    ClassObj* class_obj = GET_INSTANCE_CLASS(instance_obj);
    MethodObj* method = class_obj_get_method(class_obj, method_name);

    while (method == NULL) {
        tagged_instance_obj = GET_INSTANCE_PARENT(instance_obj);
        DEBUG_INFO(GET_TAG(tagged_instance_obj) == NULL_TAG, "method slot not in instance");
        instance_obj = GET_INSTANCE(tagged_instance_obj);
        class_obj = GET_INSTANCE_CLASS(instance_obj);
        method = class_obj_get_method(class_obj, method_name);
    }

    return method;
}


/**
 * Constant objects use c malloc to allocate memory
 */

NullObj* make_null_obj() {
    NullObj* null_obj = (NullObj*)malloc(sizeof(NullObj));
    null_obj->tag = NULL_OBJ;
    return null_obj;
}

IntObj* make_int_obj(int value) {
    IntObj* int_obj = (IntObj*)malloc(sizeof(IntObj));
    int_obj->tag = INT_OBJ;
    int_obj->value = value;
    return int_obj;
}

StringObj* make_string_obj(char* string) {
    StringObj* string_obj = (StringObj*)malloc(sizeof(StringObj));
    string_obj->tag = STRING_OBJ;
    string_obj->value = (char*)malloc(strlen(string) + 1);
    strcpy(string_obj->value, string);
    return string_obj;
}

SlotObj* make_slot_obj(StringObj* name) {
    SlotObj* slot_obj = (SlotObj*)malloc(sizeof(SlotObj));
    slot_obj->tag = SLOT_OBJ;
    slot_obj->name = name;
    return slot_obj;
}

MethodObj* make_method_obj(StringObj* name, int nargs, int nlocals, Vector* code) {
    MethodObj* method_obj = (MethodObj*)malloc(sizeof(MethodObj));
    method_obj->tag = METHOD_OBJ;
    method_obj->name = name;
    method_obj->nargs = nargs;
    method_obj->nlocals = nlocals;
    method_obj->code = code;
    method_obj->assembly = NULL;
    return method_obj;
}

ClassObj* make_class_obj() {
    ClassObj* class_obj = (ClassObj*)malloc(sizeof(ClassObj));
    class_obj->tag = CLASS_OBJ;
    class_obj->nslots = 0;
    class_obj->var_ht = make_hash_table();
    class_obj->method_ht = make_hash_table();
    return class_obj;
}

void class_obj_add_var(ClassObj* class_obj, SlotObj* slot_obj) {
    hash_table_insert(class_obj->var_ht, slot_obj->name->value, (void*)class_obj->nslots++);
}

void class_obj_add_method(ClassObj* class_obj, MethodObj* method_obj) {
    hash_table_insert(class_obj->method_ht, method_obj->name->value, method_obj);
}

int class_obj_get_var(ClassObj* class_obj, StringObj* var_name) {
    HashEntry* entry = hash_table_get(class_obj->var_ht, var_name->value);
    if (entry == NULL) {
        return -1;
    }
    return (int)entry->value;
}
 
MethodObj* class_obj_get_method(ClassObj* class_obj, StringObj* method_name) {
    HashEntry* entry = hash_table_get(class_obj->method_ht, method_name->value);
    if (entry == NULL) {
        return NULL;
    }
    return (MethodObj*)entry->value;
}

LabelObj* make_label_obj(MethodObj* method_obj, int addr) {
    LabelObj* label_obj = (LabelObj*)malloc(sizeof(LabelObj));
    label_obj->tag = LABEL_OBJ;
    label_obj->method = method_obj;
    label_obj->addr = addr;
    label_obj->assembly_addr = NULL;
    label_obj->backfill_list = make_vector();
    return label_obj;
}

void label_obj_set_assembly(LabelObj* label_obj, void* assembly_addr) {
    label_obj->assembly_addr = assembly_addr;
}

void label_obj_get_assembly(LabelObj* label_obj) {
    return label_obj->assembly_addr;
}

void label_obj_add_backfill(LabelObj* label_obj, void* backfill_addr) {
    vector_add(label_obj->backfill_list, backfill_addr);
}

void label_obj_backfill(LabelObj* label_obj, void* assembly_addr) {
    Vector* backfill_list = label_obj->backfill_list;
    for (int i = 0; i < vector_size(backfill_list); ++i) {
        void* backfill_addr = vector_get(backfill_list, i);
        *((uint64_t*)backfill_addr) = (uint64_t)assembly_addr;
    }
}

AddressObj* make_address_obj(MethodObj* method_obj, int addr) {
    AddressObj* address_obj = (AddressObj*)malloc(sizeof(AddressObj));
    address_obj->tag = ADDRESS_OBJ;
    address_obj->method = method_obj;
    address_obj->addr = addr;
    return address_obj;
}

void print_array(void* array) {
    assert(OBJ_TAG(array) == ARRAY_HEAP_OBJ);
    int length = GET_ARRAY_LENGTH(array);
    for (int i = 0; i < length; ++i) {
        void* value = GET_ARRAY_DATA(array, i);
        printf("idx:%d  %d\n", i, OBJ_TAG(value));
    }
}

void print_tagged_array(void* tagged_array) {
    // get original array pointer and check its type
    DEBUG_INFO(GET_TAG(tagged_array) != HEAP_OBJECT_TAG, "invalid type of receiver for array length");
    void* array = GET_ARRAY(tagged_array);
    print_array(array);
}