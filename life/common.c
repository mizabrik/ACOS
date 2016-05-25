#include "common.h"

#include <assert.h>
#include <semaphore.h>

#include "life.h"

void coordinator(life_t *life, life_t *tmp, unsigned steps, unsigned n_workers,
                 sem_t jobs, sem_t ready) {
  unsigned step;
  unsigned i;

  assert(n_workers > 0);
  
  for (step = 0; step < steps; ++steps) {
    for (i = 0; i < n_workers; ++i)
      sem_post(jobs);
    for (i = 0; i < n_workers; ++i)
      sem_wait(ready);

    swap_lifes(tmp, life); 
  } 
}

void worker(worker_data_t *data) {

  for (;;) {
    int x, y;

    sem_wait(data->jobs);

    for (y = data->y_begin; y < data->y_end; ++y)
      for (x = 0; x < data->new->width; ++x)
        update_cell(data->new, data->old, x, y);

    sem_post(data->ready);
  }
}

void swap_lifes(life_t *a, life_t *b) {
  life_t tmp;
  tmp = *a;
  *a = *b;
  *b = tmp;
}
