#include <stdlib.h>
#include <assert.h>
#include "../utils/utils.h"
#include "../utils/hash.h"
#include "./pool.h"

// make global table
GlobalTable* make_global_table() {
    GlobalTable* gt = (GlobalTable*)malloc(sizeof(GlobalTable));
    gt->slots = make_vector();
    gt->var_ht = make_hash_table();
    gt->method_ht = make_hash_table();
    return gt;
}

// only delete hash table of vars and methods
void delete_global_table(GlobalTable* gt) {
    delete_hash_table(gt->method_ht);
    delete_hash_table(gt->var_ht);
    free(gt);
}

// add value index to global table
void add_global_val(GlobalTable* gt, int val_index) {
    vector_add(gt->slots, (void*)val_index);
}

// add global slot to global table's variable hash table, using its name index
void add_global_var(GlobalTable* gt, char* name, int name_index) {
    hash_table_insert(gt->var_ht, name, name_index);
}

// add global method to global table's method hash table, using its name index
void add_global_method(GlobalTable* gt, char* name, int name_index) {
    hash_table_insert(gt->method_ht, name, name_index);    
}

// get global slot's name index by its name. return -1 if it does not exist
int get_global_var(GlobalTable* gt, char* name) {
    HashEntry* entry = hash_table_get(gt->var_ht, name);
    if (entry == NULL) {
        return -1;
    }
    return (int)entry->value;
}

// get global func's name index by its name. return -1 if it does not exist
int get_global_method(GlobalTable* gt, char* name) {
    HashEntry* entry = hash_table_get(gt->method_ht, name);
    if (entry == NULL) {
        return -1;
    }
    return (int)entry->value;    
}

// make pool entry
PoolEntry* make_pool_entry(Value* value, int index) {
    PoolEntry* entry = (PoolEntry*)malloc(sizeof(PoolEntry));
    entry->index = index;
    entry->value = value;
    return entry;
}

// make pool
Pool* make_pool() {
    Pool* pool = (Pool*)malloc(sizeof(Pool));
    pool->str_ht = make_hash_table();
    pool->var_ht = make_hash_table();
    pool->pool_vec = make_vector();
    pool->pool_int = make_vector();
    pool->null_entry = NULL;
    return pool;
}

void delete_pool(Pool* pool) {
    delete_vector(pool->pool_int);
    delete_hash_table(pool->str_ht);
    delete_hash_table(pool->var_ht);
    free(pool->null_entry);
    free(pool);
}

// add null value to pool and set null entry.
int add_pool_null(Pool* pool, Value* value) {
    int index = pool->pool_vec->size;
    vector_add(pool->pool_vec, value);
    PoolEntry* entry = make_pool_entry(value, index);
    pool->null_entry = entry;
    return index;
}

// add int value to pool and add int entry to pool_int
int add_pool_int(Pool* pool, Value* value) {
    int index = pool->pool_vec->size;
    vector_add(pool->pool_vec, value);

    PoolEntry* entry = make_pool_entry(value, index);
    vector_add(pool->pool_int, entry);
    return index;
}

// add string value to pool and add string entry to str_ht
int add_pool_str(Pool* pool, Value* value) {
    assert(value->tag == STRING_VAL);
    StringValue* value1 = (StringValue*)value;

    int index = pool->pool_vec->size;
    vector_add(pool->pool_vec, value1);

    PoolEntry* entry = make_pool_entry(value1, index);
    hash_table_insert(pool->str_ht, value1->value, entry);

    return index;    
}

// add variable to pool and add variable entry to var_ht
int add_pool_var(Pool* pool, Value* value, char* name) {
    assert(value->tag == SLOT_VAL);

    int index = pool->pool_vec->size;
    vector_add(pool->pool_vec, value);

    PoolEntry* entry = make_pool_entry(value, index);
    hash_table_insert(pool->var_ht, name, entry);

    return index;    
}

// add method to pool
int add_pool_method(Pool* pool, Value* value) {
    assert(value->tag == METHOD_VAL);

    int index = pool->pool_vec->size;
    vector_add(pool->pool_vec, value);

    return index;     
}

// add class to pool
int add_pool_class(Pool* pool, Value* value) {
    assert(value->tag == CLASS_VAL);

    int index = pool->pool_vec->size;
    vector_add(pool->pool_vec, value);

    return index;     
}

// get null value index from pool. 
// if null val not in constant pool, create a null val and add it to pool.
int get_pool_null(Pool* pool) {
    int index;
    PoolEntry* entry = pool->null_entry;
    if (entry == NULL) {
        Value* null_val = make_null_value();
        index = add_pool_null(pool, null_val);
    } else {
        index = entry->index;
    }
    return index;
}

// get int value index from pool.
// if int value not in pool, create a new int value and add it to pool
int get_pool_int(Pool* pool, int value) {
    PoolEntry* entry;
    IntValue* int_val;
    for (int i = 0; i < pool->pool_int->size; ++i) {
        entry = (PoolEntry*)vector_get(pool->pool_int, i);
        int_val = (IntValue*)entry->value;
        if (int_val->value == value) {
            return entry->index;
        }
    }
    // create a new int value and add it to pool
    int_val = make_int_value(value);
    int index = add_pool_int(pool, int_val);

    return index;
}


// get string value index from pool.
// if string value not in pool, create a new string value and add it to pool
int get_pool_str(Pool* pool, char* value) {
    int index;
    HashEntry* hash_entry = hash_table_get(pool->str_ht, value);
    if (hash_entry == NULL) {
        StringValue* string_val = make_string_value(value);
        index = add_pool_str(pool, string_val);
    } else {
        PoolEntry* entry = (PoolEntry*)hash_entry->value;
        index = entry->index; 
    }
    return index;
}

// get variable value index from pool
// if variable value not in pool and create set non-zero, create a new slot and add it to pool
int get_pool_var(Pool* pool, char* name, int create) { 
    int index;
    HashEntry* hash_entry = hash_table_get(pool->var_ht, name);
    if (hash_entry == NULL) {
        // not in hash table
        if (create == 0) {
            return -1;
        }
        int name_index = get_pool_str(pool, name);
        SlotValue* slot_val = make_slot_value(name_index);
        index = add_pool_var(pool, slot_val, name);
    } else {
        PoolEntry* entry = (PoolEntry*)hash_entry->value;
        index = entry->index; 
    }
    return index;
}

// make func frame
Frame* make_frame(Frame* parent) {
    Frame* frame = (Frame*)malloc(sizeof(Frame));
    frame->index = 0;
    frame->var_ht = make_hash_table();
    frame->parent = parent;
    return frame;
}

void delete_frame(Frame* frame) {
    delete_hash_table(frame->var_ht);
    free(frame);
}

// add variable to frame
int add_frame_var(Frame* frame, char* name) {
    int index = frame->index++;
    hash_table_insert(frame->var_ht, name, index);
    return index;
}

// get variable from frame.
// if not exist, return -1
int get_frame_var(Frame* frame, char* name) {
    HashEntry* entry = hash_table_get(frame->var_ht, name);
    if (entry == NULL) {
        return -1;
    }
    return (int)entry->value;
}

void itoa(int i, char* s) {
    char* b = s;
    while (i > 0) {
        *(s++) = (i % 10) + '0';
        i = i / 10;
    }
    *s = '\0';
    s--;
    char c;
    while (b < s) {
        c = *s;
        *s = *b;
        *b = c;
        s--;
        b++;
    }
}

// allocate label name
char* alloc_label(ControlTag tag, int index) {
    char num[11];
    itoa(index, num);
    char* label;
    switch (tag) {
        case TEST: {
            label = (char*)malloc(sizeof(char) * (5 + strlen(num)));
            strcpy(label, "test");
            strcat(label, num);
            break;
        }
        case CONSEQ: {
            label = (char*)malloc(sizeof(char) * (7 + strlen(num)));
            strcpy(label, "conseq");
            strcat(label, num);
            break;
        }
        case LOOP: {
            label = (char*)malloc(sizeof(char) * (5 + strlen(num)));
            strcpy(label, "loop");
            strcat(label, num);
            break;
        }
        case END: {
            label = (char*)malloc(sizeof(char) * (4 + strlen(num)));
            strcpy(label, "end");
            strcat(label, num);
            break;
        }
        default: {
            assert(0);
        }
    }
    return label;
}

// allocate entry name
char* alloc_entry(int index) {
    char num[11];
    itoa(index, num);
    char* entry_name = (char*)malloc(sizeof(char) * (6 + strlen(num)));
    strcpy(entry_name, "entry");
    strcat(entry_name, num);
    return entry_name;
}