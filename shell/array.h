#ifndef ARRAY_H_
#define ARRAY_H_

#include <stddef.h>

struct array {
  void **data;
  int size;

  int capacity;
};
typedef struct array array_t;

void array_new(array_t *array);
void array_add(array_t *array, const void *element, size_t size);
void array_delete(array_t *array);

#endif // ARRAY_H_
