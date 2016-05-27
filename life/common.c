#include "common.h"

#include <assert.h>
#include <semaphore.h>
#include <stdio.h>

#include "life.h"

void coordinator(life_t *life, life_t *tmp, unsigned steps, unsigned n_workers,
                 sem_t* ready, sem_t *nexts) {
  unsigned step;
  unsigned i;

  assert(n_workers > 0);
  //printf("steps: %d, n_workers: %d\n", steps, n_workers);
  
  for (step = 0; step < steps; ++step) {
    for (i = 0; i < n_workers; ++i) {
      sem_post(&nexts[i]);
      //printf("sem_post(jobs)\n");
    }
    for (i = 0; i < n_workers; ++i) {
      //printf("i: %d, step: %d\n", i, step);
      //printf("sem_wait(ready): start\n");
      sem_wait(ready);
      //printf("sem_wait(ready): done\n");
    }

    swap_lifes(tmp, life); 
  } 
}

void worker(worker_data_t *data) {
  unsigned step;
  /*printf("steps: %d\n", data->steps);
  printf("y_begin: %d\n", data->y_begin);
  printf("y_end: %d\n", data->y_end);*/

  for (step = 0; step < data->steps; ++step) {
    int x, y;
    //printf("sem_wait(data->next): start\n");
    sem_wait(data->next);
    //printf("sem_wait(data->next): done\n");

    //life_print(data->old, stdout);
    for (y = data->y_begin; y < data->y_end; ++y) {
      for (x = 0; x < data->new->width; ++x) {
        update_cell(data->new, data->old, x, y);
        //printf("x: %d; y: %d\n", x, y);
        //life_print(data->new, stdout);
        //life_print(data->old, stdout);
      }
    }

    sem_post(data->ready);
    //printf("sem_post(data->ready)\n");
  }
}

void swap_lifes(life_t *a, life_t *b) {
  life_t tmp;
  tmp = *a;
  *a = *b;
  *b = tmp;
}
