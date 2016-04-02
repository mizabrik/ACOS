CC=gcc
LDLIBS=-ldl
CFLAGS=-fPIC

.PHONY: all
all: main plugins

main: main.o

plugins: hello.o time.o silent.o
	mkdir -p plugins
	$(CC) -shared -o plugins/hello.so hello.o
	$(CC) -shared -o plugins/time.so time.o
	$(CC) -shared -o plugins/silent.so silent.o
	touch plugins/fake.so

.PHONY: clean
clean:
	rm -rf main *.o plugins
