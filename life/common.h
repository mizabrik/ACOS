#ifndef COMMON_H_
#define COMMON_H_

#include <semaphore.h>

#include "life.h"

struct worker_data {

  life_t *old;
  life_t *new;

  unsigned steps;
  int y_begin;
  int y_end;
  sem_t *ready;
  sem_t *next;
};
typedef struct worker_data worker_data_t;


/* requires */
void coordinator(life_t *life, life_t *tmp, unsigned steps, unsigned n_workers,
                 sem_t *ready, sem_t **nexts);

void worker(struct worker_data *data);

void swap_lifes(life_t *a, life_t *b);

#endif /* COMMON_H_ */
