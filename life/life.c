#include "life.h"

#include <stdio.h>
#include <stdlib.h>

int life_new(life_t *life, int width, int height) {
  life->width = width;
  life->height = height;
  life->field = (char *) malloc(width * height * sizeof(char));

  return 0;
}

int cell_id(life_t *life, int x, int y) {
  if (x % life->width >= 0)
    x = x % life->width;
  else
    x = life->width - x % life->width;

  if (y % life->height >= 0)
    y = y % life->height;
  else
    y = life->height - y % life->height;

  return y * life->width + x;
}

char * get_cell(life_t *life, int x, int y) {
  return &life->field[cell_id(life, x, y)];
}

static int count_neighbours(life_t *life, int x, int y) {
  return *get_cell(life, x - 1, y - 1)
       + *get_cell(life, x, y - 1)
       + *get_cell(life, x + 1, y - 1)
       + *get_cell(life, x + 1, y)
       + *get_cell(life, x + 1, y + 1)
       + *get_cell(life, x, y + 1)
       + *get_cell(life, x - 1, y + 1)
       + *get_cell(life, x - 1, y);
}

char get_state(life_t *life, int x, int y) {
  int neighbours;
  char cell;
  
  neighbours = count_neighbours(life, x, y);
  cell = *get_cell(life, x, y);
  if (cell && (neighbours < 2 || 3 < neighbours)) {
    return 0;
  } else if (!cell && neighbours == 3) {
    return 1;
  } else {
    return cell;
  }
}

int life_read(life_t *life, FILE *input) {
  int i, j;

  fscanf(input, "%dx%d\n", &life->width, &life->height);

  for (i = 0; i < life->height; ++i) {
    for (j = 0; j < life->width; ++j) {
      int cell = fgetc(input);
      if (cell >= 0) {
        *get_cell(life, j, i) =  cell == 'X' || cell == 'x';
      }
    }
    fgetc(input); /* \n */
  }

  return 0;
}

int print_life(life_t *life, FILE *output) {
  int i, j;

  fprintf(output, "%dx%d\n", life->width, life->height);
  for (i = 0; i < life->height; ++i) {
    for (j = 0; j < life->width; ++j) {
      if(fputc(*get_cell(life, j, i) ? 'X' : '.', output) < 0) {
        return -1;
      }
    }
    fputc('\n', output);
  }

  return 0;
}

void life_destroy(life_t *life) {
  free(life->field);
}
