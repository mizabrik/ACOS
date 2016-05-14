#ifndef COMMAND_H_
#define COMMAND_H_

#include "array.h"

struct command {
  array_t programs;
  int input;
  int output;
};
typedef struct command command_t;

int parse(command_t *command, const char *input);

#endif // COMMAND_H_
