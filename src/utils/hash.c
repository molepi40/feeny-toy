#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "utils.h"
#include "hash.h"

const size_t default_capacity = 16;
const uint64_t FNV_offset_basis = 0xcbf29ce484222325;
const uint64_t FNV_prime = 0x100000001b3;

HashEntry* make_hash_entry(char* key, void* value) {
    HashEntry* hash_entry = (HashEntry*)malloc(sizeof(HashEntry));
    if (hash_entry == NULL) {
        return NULL;
    }
    hash_entry->key = key;
    hash_entry->value = value;
    return hash_entry;
}

HashTable* make_hash_table() {
    HashTable* hash_table = (HashTable*)malloc(sizeof(HashTable));
    if (hash_table == NULL) {
        return NULL;
    }
    HashEntry** table = (HashEntry**)calloc(default_capacity, sizeof(HashEntry*));
    if (table == NULL) {
        free(hash_table);
        return NULL;
    }
    hash_table->table = table;
    hash_table->size = 0;
    hash_table->capacity = default_capacity;
    return hash_table;
}

// FNV-1a hash algorithm https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash
uint64_t fnv1a_hash(char* key, uint64_t capacity) {
    uint64_t hash_val = FNV_offset_basis;
    for (int i = 0; key[i] != '\0'; ++i) {
        hash_val ^= (uint64_t)key[i];
        hash_val *= FNV_prime;
    }
    return hash_val & (capacity - 1);
}

// return NULL if not find a position
HashEntry** hash_table_find(HashEntry** table, uint64_t capacity, char* key) {
    size_t index = (size_t)fnv1a_hash(key, capacity);
    HashEntry** p = table + index;
    while (*p) {
        if (strcmp((*p)->key, key) == 0) {
            return p;
        }
        ++p;
        if (p >= table + capacity) {
            p = table;
        }
        if (p == table + index) {
            return NULL;
        }
    }
    return p;    
}

void hash_table_put(HashEntry** table, uint64_t capacity, HashEntry* entry) {
    HashEntry** p = hash_table_find(table, capacity, entry->key);
    *p = entry;
    return;
}

void ensure_hash_table_capacity(HashTable* hash_table) {
    if (hash_table->size < hash_table->capacity) {
        return;
    }
    size_t new_capacity = hash_table->capacity << 1;
    HashEntry** new_table = (HashEntry**)calloc(new_capacity, sizeof(HashEntry*));

    HashEntry** old_table = hash_table->table;
    for (size_t i = 0; i < hash_table->capacity; ++i) {
        if (old_table[i]) {
            hash_table_put(new_table, (uint64_t)new_capacity, old_table[i]);
        }
    }
    free(hash_table->table);
    hash_table->table = new_table;
    hash_table->capacity = new_capacity;
}

void hash_table_insert(HashTable* hash_table, char* key, void* value) {
    ensure_hash_table_capacity(hash_table);
    // ensure_hash_table_capacity confirm that p must not be NULL
    HashEntry** p = hash_table_find(hash_table->table, hash_table->capacity, key);
    if (*p) {
        (*p)->value = value;
    } else {
        *p = make_hash_entry(key, value);
    }
    hash_table->size++;
}

void hash_table_set(HashTable* hash_table, char* key, void* value) {
    HashEntry** p = hash_table_find(hash_table->table, hash_table->capacity, key);
    if (p == NULL || *p == NULL) {
        DEBUG_INFO(1, "entry not in hash table");
    } else {
        (*p)->value = value;
    }
}

HashEntry* hash_table_get(HashTable* hash_table, char* key) {
    HashEntry** p = hash_table_find(hash_table->table, hash_table->capacity, key);
    if (p == NULL) {
        // hash table full
        return NULL;
    }

    return *p;
}

void delete_hash_entry(HashEntry* entry) {
    // free(entry->key);
    // free(entry->value);
    free(entry);
}

void delete_hash_table(HashTable* hash_table) {
    HashEntry** p = hash_table->table;
    while (p < hash_table->table + hash_table->capacity) {
        if (*p) {
            delete_hash_entry(*p);
        }
        ++p;
    }
    free(hash_table->table);
    free(hash_table);
}