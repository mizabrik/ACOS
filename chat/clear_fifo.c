#include <sys/stat.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SEM_WRITE "sem_write"
#define SEM_FIRST_PROCESS "sem_first_process"
#define SEM_SECOND_PROCESS "sem_second_process"

int main(int args, char* argv[]) {

	int fifo_fd;
	int is_first = 0;
	mkfifo(argv[1], 0600);
	fifo_fd = open(argv[1], O_RDWR, O_CREAT);
	sem_t* sem_write;
	sem_t* sem_first;
	sem_t* sem_second;
	sem_write = sem_open(SEM_WRITE, O_CREAT, 0600, 1);
	sem_first = sem_open(SEM_FIRST_PROCESS, O_CREAT, 0600, 0);
	sem_second = sem_open(SEM_SECOND_PROCESS, O_CREAT, 0600, 0);

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
