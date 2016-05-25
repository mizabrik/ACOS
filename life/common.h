#ifndef COMMON_H_
#define COMMON_H_

#include <semaphore.h>
#include <pthread.h>

#include "life.h"

struct worker_data {
  pthread_t thread;

  life_t *old;
  life_t *new;

  int y_begin;
  int y_end;
  sem_t *jobs;
  sem_t *ready; 
};
typedef struct worker_data worker_data_t;


/* requires */
void coordinator(life_t *life, life_t *tmp, unsigned steps, unsigned n_workers,
                 sem_t jobs, sem_t ready);

void worker(struct worker_data *data);

void swap_lifes(life_t *a, life_t *b);

#endif /* COMMON_H_ */
