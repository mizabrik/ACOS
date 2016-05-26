#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <error.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"
#include "life.h"

#define JOBS_NAME "/sem_jobs"
#define READY_NAME "/sem_ready"
#define NEXT_NAME "/sem_next_"

int main(int argc, char* argv[]) {
  if(argc < 4) {
    error(EXIT_FAILURE, 0, "No field, number of threads or number of steps provided");
  }

  int number_of_threads, number_of_steps;

  number_of_threads = atoi(argv[2]);
  number_of_steps = atoi(argv[3]);
  FILE *file = fopen(argv[1], "r");
  life_t life;
  int rc;
  rc = life_read(&life, file);
  if(rc < 0) {
    error(EXIT_FAILURE, 0, "Can't read from file\n");
  }

  life_print(&life, stdout);
  life_t tmp;
  life_new(&tmp, life.width, life.height);

  int i;
  sem_t *ready;
  sem_t *nexts[number_of_threads];
  errno = 0;
  ready = sem_open(READY_NAME, O_CREAT | O_EXCL, 0600, 0);
  sem_unlink(READY_NAME);
  if(errno) {
    //perror("First");
    error(EXIT_FAILURE, 0, "Problems with syncronisation");
  }
  for(i = 0; i < number_of_threads; i++) {
    char name[255];
    snprintf(name, 255, "%s%d", NEXT_NAME, i);
    errno = 0;
    nexts[i] = sem_open(name, O_CREAT | O_EXCL, 0600, 0);
    sem_unlink(name);
    if(errno) {
      //perror("In cycle");
      error(EXIT_FAILURE, 0, "Problems with syncronisation");
    }
  }

  int strings_on_thread = life.height / number_of_threads;
  
  pthread_t threads[number_of_threads];
  struct worker_data *wds[number_of_threads];

  for(i = 0; i < number_of_threads-1; i++) {
    wds[i] = (struct worker_data*) malloc (sizeof(struct worker_data));
    wds[i]->new = &tmp;
    wds[i]->old = &life;
    wds[i]->y_begin = i*strings_on_thread;
    wds[i]->y_end = (i+1)*strings_on_thread;
    wds[i]->steps = number_of_steps;
    wds[i]->next = nexts[i];
    wds[i]->ready = ready;
    if(pthread_create(&threads[i], NULL, (void * (*)(void*))worker, wds[i]) != 0) {
      error(EXIT_FAILURE, 0, "Problems with threads");
    }
  }

  //printf("number_of_threads = %d\n", number_of_threads);
  //printf("strings_on_thread = %d\n", strings_on_thread);

  wds[number_of_threads-1] = (struct worker_data*) malloc (sizeof(struct worker_data));
  wds[number_of_threads-1]->new = &tmp;
  wds[number_of_threads-1]->old = &life;
  wds[number_of_threads-1]->y_begin = (number_of_threads-1)*strings_on_thread;
  wds[number_of_threads-1]->y_end = (number_of_threads)*(strings_on_thread + life.height % number_of_threads);
  wds[number_of_threads-1]->steps = number_of_steps;
  wds[number_of_threads-1]->next = nexts[number_of_threads-1];
  wds[number_of_threads-1]->ready = ready;
  if(pthread_create(&threads[number_of_threads-1], NULL, (void * (*)(void*))worker, wds[number_of_threads-1]) != 0) {
    error(EXIT_FAILURE, 0, "Problems with threads");
  }

  coordinator(&life, &tmp, number_of_steps, number_of_threads, ready, nexts);

  life_print(&life, stdout);

  for(i = 0; i < number_of_threads-1; i++) {
    free(wds[i]);
  }

  for(i = 0; i < number_of_threads-1; i++) {
    if(pthread_join(threads[i], NULL) != 0) {
      error(EXIT_FAILURE, 0, "Can't join threads");
    }
  }

  return EXIT_SUCCESS;
}
