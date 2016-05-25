#ifndef LIFE_H_
#define LIFE_H_

#include <stdio.h>

typedef struct {
  int width;
  int height;

  char *field;
} life_t;

int life_new(life_t *life, int width, int height);

int cell_id(life_t *life, int x, int y);

char * get_cell(life_t *life, int x, int y);

void update_cell(life_t *life, life_t *old_life, int x, int y);

int life_read(life_t *life, FILE *input);

int life_print(life_t *life, FILE *output);

void life_destroy(life_t *life);

#endif // LIFE_H_
