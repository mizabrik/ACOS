#include "args.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_CAPACITY 4

void args_new(args_t *args) {
  args->argv = (char **) malloc((DEFAULT_CAPACITY + 1) * sizeof(char *));
  args->argv[0] = NULL;
  args->argc = 0;
  args->capacity = DEFAULT_CAPACITY;
}

void args_add(args_t *args, const char *arg) {
  if (args->argc == args->capacity) {
    args->argv = (char **) realloc(args->argv,
                                   (args->capacity + 1) * 2 * sizeof(char *));
    args->capacity *= 2;
  }

  args->argv[args->argc] = (char *) malloc(strlen(arg));
  strcpy(args->argv[args->argc], arg);
  ++args->argc;
  args->argv[args->argc] = NULL;
}

void args_exterminate(args_t *args) {
  int i;

  for (i = 0; i < args->argc; ++i)
    free(args->argv[i]);
}
