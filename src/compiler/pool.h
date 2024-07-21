#ifndef POOL_H
#define POOL_H
#include "../utils/utils.h"
#include "../utils/hash.h"
#include "./ast.h"
#include "./bytecode.h"

typedef enum {
    TEST,
    CONSEQ,
    END,
    LOOP
} ControlTag;

typedef struct {
    Vector* slots;
    HashTable* var_ht;
    HashTable* method_ht;
} GlobalTable;

typedef struct {
    int index;
    Value* value;
} PoolEntry;

typedef struct {
    HashTable* str_ht;
    HashTable* var_ht;

    Vector* pool_vec;
    Vector* pool_int;
    PoolEntry* null_entry;
} Pool;

typedef struct {
    int index;
    HashTable* var_ht;
    void* parent;
} Frame;

GlobalTable* make_global_table();
void delete_global_table(GlobalTable* gt);
void add_global_val(GlobalTable* gt, int val_index);
int get_global_var(GlobalTable* gt, char* name);
void add_global_var(GlobalTable* gt, char* name, int name_index);
int get_global_method(GlobalTable* gt, char* name);
void add_global_method(GlobalTable* gt, char* name, int name_index);

Pool* make_pool();
void delete_pool(Pool* pool);
int add_pool_null(Pool* pool, Value* value);
int add_pool_int(Pool* pool, Value* value);
int add_pool_str(Pool* pool, Value* value);
int add_pool_var(Pool* pool, Value* value, char* name);
int add_pool_method(Pool* pool, Value* value);
int add_pool_class(Pool* pool, Value* value);
int get_pool_int(Pool* pool, int value);
int get_pool_str(Pool* pool, char* value);
int get_pool_var(Pool* pool, char* name, int create);
int get_pool_null(Pool* pool);

Frame* make_frame(Frame* parent);
void delete_frame(Frame* frame);
int add_frame_var(Frame* frame, char* name);
int get_frame_var(Frame* frame, char* name);

char* alloc_label(ControlTag tag, int index);
char* alloc_entry(int index);

#endif