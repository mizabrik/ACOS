CFLAGS = -g -Wall -Wextra
LDLIBS = -lpthread -lrt

all: threads forks dommy udp

threads: threads.o life.o common.o

forks: forks.o life.o common.o

udp: udp.o life.o common.o

dummy: dummy.o life.o common.o

.PHONY :
clean: $(RM) *.o dummy threads
