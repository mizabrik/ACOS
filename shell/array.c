#include "array.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_CAPACITY 4

void array_new(array_t *array) {
  array->data = (void **) malloc((DEFAULT_CAPACITY + 1) * sizeof(char *));
  array->data[0] = NULL;
  array->size = 0;
  array->capacity = DEFAULT_CAPACITY;
}

void array_add(array_t *array, const void *element, size_t size) {
  if (array->size == array->capacity) {
    array->data = (void **) realloc(array->data,
                                   (array->capacity + 1) * 2 * sizeof(char *));
    array->capacity *= 2;
  }

  array->data[array->size] = (void *) malloc(size);
  memcpy(array->data[array->size], element, size);
  ++array->size;
  array->data[array->size] = NULL;
}

void array_delete(array_t *array) {
  int i;

  for (i = 0; i < array->size; ++i)
    free(array->data[i]);
  free(array->data);
}
