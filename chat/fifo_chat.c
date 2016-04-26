#include <sys/stat.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SEM_WRITE "/sem_write"
#define SEM_FIRST_PROCESS "/sem_first_process"
#define SEM_SECOND_PROCESS "/sem_second_process"


int get_line(char **line, FILE *file) {
  int c;
  int size = 0;
  int capacity = 42; // default buffer size

  *line = (char *) malloc(capacity * sizeof(char));
  if (!*line) {
    return -1;
  }

  while ((c = fgetc(file)) != EOF && c != '\n') {
    (*line)[size] = c;
    ++size;
    if (size == capacity) {
      char *tmp;
      capacity *= 2;
      tmp = (char *) realloc(*line, capacity * sizeof(char));
      if (!tmp) {
        return -1;
      }
      *line = tmp;
    }
  }
  (*line)[size] = '\0';

  return size;
}

struct my_fifo {
	sem_t* sem_write;
	sem_t* sem_read;
	int fifo_fd;
};

static void *read_from_fifo(__attribute__((unused)) void *void_ptr) {
	do {
		struct my_fifo* fifo;
		fifo = (struct my_fifo*) void_ptr;
		sem_wait(fifo->sem_read);
		char* c;
		c = (char*) malloc(1);
		read(fifo->fifo_fd, c, 1);
		putchar(c[0]);
		if(c[0] == '\n') {
			sem_post(fifo->sem_write);
		}
		free(c);
	} while(!feof(stdin));
	return NULL;
}

int main(int argc, char *argv[]) {
	int fifo_fd;
	int is_first = 0;
	mkfifo(argv[1], 0600);
	fifo_fd = open(argv[1], O_RDWR, O_CREAT);
	if (errno == EEXIST) {
		is_first = 1;
	}
	sem_t* sem_write;
	sem_t* sem_first;
	sem_t* sem_second;
	sem_write = sem_open(SEM_WRITE, O_CREAT, 0600, 1);
	sem_first = sem_open(SEM_FIRST_PROCESS, O_CREAT, 0600, 0);
	sem_second = sem_open(SEM_SECOND_PROCESS, O_CREAT, 0600, 0);
	pthread_t thread;

	struct my_fifo fifo;
	fifo.sem_write = sem_write;
	fifo.fifo_fd = fifo_fd;
	if(is_first == 0) {
		fifo.sem_read = sem_first;
	} else {
		fifo.sem_read = sem_second;
	}

	if(pthread_create(&thread, NULL, read_from_fifo, &fifo) != 0) {
		return EXIT_FAILURE;
	}

	do {
		char* str;
		get_line(&str, stdin);
		sem_wait(sem_write);
		char* c;
		c = (char*) malloc(1);
		int i;
		for(i = 0; i < strlen(str); ++i) {
			c[0] = str[i];
			write(fifo_fd, c, 1);
			if(is_first == 0) {
				sem_post(sem_second);
			} else {
				sem_post(sem_first);
			}
		}
		c[0] = '\n';
		write(fifo_fd, c, 1);
		if(is_first == 0) {
			sem_post(sem_second);
		} else {
			sem_post(sem_first);
		}
	} while(!feof(stdin));

	if(pthread_join(thread, NULL) != 0) {
		return EXIT_FAILURE;
	}
	sem_close(sem_write);
	sem_close(sem_first);
	sem_close(sem_second);
	sem_unlink(SEM_WRITE);
	sem_unlink(SEM_FIRST_PROCESS);
	sem_unlink(SEM_SECOND_PROCESS);
	unlink(argv[1]);
	close(fifo_fd);

	return EXIT_SUCCESS;
}
