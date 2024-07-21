#ifndef UTILS_H
#define UTILS_H
#include <assert.h>

#ifndef DEBUG_INFO(pred, info)
#define DEBUG_INFO(pred, info) \
if (pred) { \
  printf("file %s line %d func %s: %s\n", __FILE__, __LINE__, __FUNCTION__, info); \
  fflush(stdout); \
  assert(0); \
}
#endif

int max (int a, int b);
int min (int a, int b);
void print_string (char* str);

//============================================================
//===================== VECTORS ==============================
//============================================================

typedef struct {  
  int size;
  int capacity;
  void** array;
} Vector;

Vector* make_vector ();
Vector* make_vector_custom(int capacity);
void delete_vector(Vector* v);
int vector_size(Vector* v);
int vector_empty(Vector* v);
void vector_add (Vector* v, void* val);
void* vector_pop (Vector* v);
void* vector_peek (Vector* v);
void vector_clear (Vector* v);
void vector_free (Vector* v);
void* vector_get (Vector* v, int i);
void vector_set (Vector* v, int i, void* x);
void vector_set_length (Vector* v, int len, void* x);

#endif
