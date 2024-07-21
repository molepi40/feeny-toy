#ifndef MEM_H
#define MEM_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void* from_space;
    void* to_space;
} MemManager;

void initilize_mem_manager();
void* halloc(int nbytes);
void* garbage_collector();
void scan_root_set();
void* get_post_gc_ptr(void* tagged_obj);

#endif