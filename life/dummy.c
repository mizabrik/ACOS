#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>

#include "life.h"
#include "common.h"

int main(int argc, char *argv[]) {
  if(argc < 3) {
    error(EXIT_FAILURE, 0, "The number of steps or file aren't transferred \n");
  }

  int number_of_steps;
  number_of_steps = atoi(argv[2]);

  FILE* file;
  file = fopen(argv[1], "r");
  life_t life;
  life.field = NULL;
  int rc;
  rc = life_read(&life, file);
  if(rc < 0) {
    error(EXIT_FAILURE, 0, "Error reading from file");
  }
  life_print(&life, stdout);

  int i;
  int x;
  int y;
  life_t updated_life;
  life_new(&updated_life, life.width, life.height);
  for(i = 0; i < number_of_steps; ++i) {
    for(y = 0; y < life.height; ++y) {
      for(x = 0; x < life.width; ++x) {
        update_cell(&updated_life, &life, x, y);
      }
    }
    swap_lifes(&life, &updated_life);
  }
  life_print(&life,  stdout);

  return EXIT_SUCCESS;
}
