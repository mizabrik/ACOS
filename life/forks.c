#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <stdint.h>

#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "life.h"
#include "common.h"

struct shared {
  size_t length;
  uint8_t *base;
  sem_t *ready;
  sem_t *next;
  char *field1;
  char *field2;
  life_t *life;
  life_t *tmp;
};

int get_shm(int width, int height, unsigned n_workers, struct shared *mem) {
  int fd;
  size_t length;
  off_t off = 0;
  int prot = PROT_READ | PROT_WRITE;

  fd = shm_open("/LifeWantsToBeUnique", O_RDWR | O_CREAT | O_EXCL, 0600);
  if (fd < 0)
    return -1;
  shm_unlink("/LifeWantsToBeUnique");
  length = 2 * sizeof(life_t) /* two lifes */
         + 2 * width * height /* two buffers */
         + (n_workers + 1) * sizeof(sem_t); /* semaphores */
  ftruncate(fd, length);
  mem->base = (uint8_t *) mmap(NULL, length, prot, MAP_SHARED, fd, 0);

  mem->ready = (sem_t *) (mem->base + off);
  off += sizeof(sem_t);
  mem->next = (sem_t *) (mem->base + off);
  off += sizeof(sem_t) * n_workers;
  mem->field1 = (char *) (mem->base + off);
  off += sizeof(char) * width * height;
  mem->field2 = (char *) (mem->base + off);
  off += sizeof(char) * width * height;
  mem->life = (life_t *) (mem->base + off);
  off += sizeof(life_t);
  mem->tmp = (life_t *) (mem->base + off);
  off += sizeof(life_t);
  mem->length = length;
  close(fd);

  return 0;
}

int main(int argc, char **argv) {
  int ret = 0;
  unsigned n_workers = 4;
  FILE *input;
  unsigned i;
  worker_data_t *data;
  pid_t *children;
  int steps = 0;
  int width, height;
  struct shared mem;
  int block_size;

  life_t *life, *tmp;

  if (argc != 3) {
    error(0, 0, "usage: %s MAP STEPS", argv[0]);
    ret = -1;
    goto main_none;
  }

  input = fopen(argv[1], "r");
  if (input == NULL) {
    error(0, errno, "could not open %s", argv[1]);
  }
  fscanf(input, "%dx%d\n", &width, &height);
  fseek(input, 0, SEEK_SET);

  if (get_shm(width, height, n_workers, &mem)) {
    error(0, errno, "could not get shared memory");
    ret = -1;
    goto main_none;
  }
  life = mem.life;
  tmp = mem.tmp;
  life->width = tmp->width = width;
  life->height = tmp->height = height;
  life->field = mem.field1;
  tmp->field = mem.field2;

  life_read(life, input);

  steps = atoi(argv[2]);

  assert(steps > 0);

  sem_init(mem.ready, 1, 0);

  data = (worker_data_t *) malloc(sizeof(worker_data_t) * n_workers);
  children = (pid_t *) malloc(sizeof(pid_t) * n_workers);

  block_size = life->height / n_workers;
  for (i = 0; i < n_workers; ++i) {
    pid_t pid;
    sem_init(&mem.next[i], 1, 0);
    data[i].old = life;
    data[i].new = tmp;
    data[i].next = &mem.next[i];
    data[i].ready = mem.ready;
    data[i].y_begin = i * block_size;
    data[i].y_end = i + 1 == n_workers ? life->height : data[i].y_begin + block_size;
    data[i].steps = steps;
    pid = fork();
    if (pid > 0) {
      children[i] = pid;
    } else if (pid == 0) {
      worker(&(data[i]));
      return 0;
    }
  }
  data[n_workers - 1].y_end = life->height;

  coordinator(life, tmp, steps, n_workers, mem.ready, mem.next);

  for (i = 0; i < n_workers; ++i) {
    waitpid(children[i], NULL, 0);
  }

  life_print(life, stdout);


main_input:
  munmap(mem.base, mem.length);
  fclose(input);
  free(data);
  free(children);
main_none:
  return ret;
}
