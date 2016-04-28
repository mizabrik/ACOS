#include <sys/stat.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

//#define sem_write_name "/sem_write"
//#define sem_first_name "/sem_first_process"
//#define sem_second_name "/sem_second_process"
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

static void *read_from_fifo(void *void_ptr) {
  struct my_fifo *fifo;
  fifo = (struct my_fifo*) void_ptr;
  do {
    sem_wait(fifo->sem_read);
    char c;
    read(fifo->fifo_fd, &c, 1);
    putchar(c);
    if(c == '\n') {
      sem_post(fifo->sem_write);
    }
  } while(1);//!feof(stdin));

  return NULL;
}

int main(int argc, char *argv[]) {
  if(argc != 2) {
    error(EXIT_FAILURE, 0, "Wrong number of arguments.");
  }

  int fifo_fd;
  int process_number = 1;
  struct stat fifo_stat;
  int stat_res;
  stat_res = stat(argv[1], &fifo_stat);
    
  if(stat_res == 0) {
    if(!S_ISFIFO(fifo_stat.st_mode)) {
      error(EXIT_FAILURE, 0, "%s exists.", argv[1]);
    }
  } else if (errno == ENOENT) {
    process_number = 0;
  } else {
      error(EXIT_FAILURE, 0, "Something wrong with %s.", argv[1]);
  }

  if(process_number == 0) {
    mkfifo(argv[1], 0600);
  }

  errno = 0;

  fifo_fd = open(argv[1], O_RDWR, O_CREAT);
  if (errno != 0 && errno != EEXIST) {
    error(EXIT_FAILURE, 0, "Cannot open %s", argv[1]);
  }

  sem_t* sem_write;
  sem_t* sem_first;
  sem_t* sem_second;

  char sem_write_name[255];
  char sem_first_name[255];
  char sem_second_name[255];

  snprintf(sem_write_name, 255, "%s%s", SEM_WRITE, argv[1]);
  snprintf(sem_first_name, 255, "%s%s", SEM_FIRST_PROCESS, argv[1]);
  snprintf(sem_second_name, 255, "%s%s", SEM_SECOND_PROCESS, argv[1]);

  errno = 0;
  if(process_number == 0) {
    sem_unlink(sem_write_name);
    sem_unlink(sem_first_name);
    sem_unlink(sem_second_name);
    errno = 0;
    sem_write = sem_open(sem_write_name, O_CREAT | O_EXCL, 0600, 0);
    sem_first = sem_open(sem_first_name, O_CREAT | O_EXCL, 0600, 0);
    sem_second = sem_open(sem_second_name, O_CREAT | O_EXCL, 0600, 0);
    if (errno) {
      error(EXIT_FAILURE, 0, "Problems with synchronisation.");
    }
  } else {
    sem_write = sem_open(sem_write_name, 0);
    sem_first = sem_open(sem_first_name, 0);
    sem_second = sem_open(sem_second_name, 0);
    int sem_val;
    int not_correct_values = 0;
    sem_getvalue(sem_write, &sem_val);
    not_correct_values += sem_val;
    sem_getvalue(sem_first, &sem_val);
    not_correct_values += sem_val;
    sem_getvalue(sem_second, &sem_val);
    not_correct_values += sem_val;
    sem_post(sem_write);
    sem_unlink(sem_write_name);
    sem_unlink(sem_first_name);
    sem_unlink(sem_second_name);
    unlink(argv[1]);
    if(not_correct_values || errno) {
      error(EXIT_FAILURE, 0, "Problems with synchronisation.");
    }
  }
  errno = 0;

  struct my_fifo *fifo;
  fifo = (struct my_fifo*) malloc (sizeof(struct my_fifo));
  fifo->sem_write = sem_write;
  fifo->fifo_fd = fifo_fd;
  if(process_number == 0) {
    fifo->sem_read = sem_first;
  } else {
    fifo->sem_read = sem_second;
  }
    
  pthread_t thread;

  if(pthread_create(&thread, NULL, read_from_fifo, fifo) != 0) {
    error(EXIT_FAILURE, 0, "Problems with threads.");
  }

  do {
    char* str;
    get_line(&str, stdin);
    sem_wait(sem_write);
    char* c = str;
    int i;
    for(i = 0; i < strlen(str); ++i) {
      write(fifo_fd, c, 1);
      ++c;
      if(process_number == 0) {
        sem_post(sem_second);
      } else {
        sem_post(sem_first);
      }
    }
    *c = '\n';
    write(fifo_fd, c, 1);
    if(process_number == 0) {
      sem_post(sem_second);
    } else {
      sem_post(sem_first);
    }
    int t;
  } while(!feof(stdin));

  sem_close(sem_write);
  sem_close(sem_first);
  sem_close(sem_second);
  sem_unlink(sem_write_name);
  sem_unlink(sem_first_name);
  sem_unlink(sem_second_name);
  close(fifo_fd);
  unlink(argv[1]);
  free(fifo);

  return EXIT_SUCCESS;
}
