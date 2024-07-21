#ifndef HASH_H
#define HASH_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char* key;
    void* value;
} HashEntry;

typedef struct {
    size_t capacity;
    size_t size;
    HashEntry** table; 
} HashTable;

// HashEntry* make_hash_entry(const char* key, void* value);
// void delete_hash_entry(HashEntry* entry);

HashTable* make_hash_table();
void hash_table_insert(HashTable* hash_table, char* key, void* value);
void hash_table_set(HashTable* hash_table, char* key, void* value);
HashEntry* hash_table_get(HashTable* hash_table, char* key);
void delete_hash_table(HashTable* hash_table);


#endif