CFLAGS = -g -Wall -Wextra
LDLIBS = -lpthread -lrt

EXECUATBLES=threads forks dummy udp_client udp_server

all: $(EXECUATBLES)

threads: threads.o life.o common.o

forks: forks.o life.o common.o

udp_server: udp_server.o life.o common.o

udp_client: udp_client.o life.o common.o

dummy: dummy.o life.o common.o

.PHONY :
clean:
	$(RM) *.o $(EXECUATBLES)
