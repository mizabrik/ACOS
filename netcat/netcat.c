#include <error.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

#define POLL_NFDS 2
#define POLL_TIME 1000

#define DEFAULT_CAPACITY 80
#define READ_SIZE 20

struct input {
  size_t capacity;
  size_t read;
  char *str;
};

volatile sig_atomic_t finished = 0;

void termination_handler(int sig);
int get_addr(struct sockaddr_in *addr, const char *host, const char *port);
int append_input(struct input *input, int fd);
void set_nonblocking(int fd);

int main(int argc, char **argv)
{
  int result = 0;

  int serverfd;

  struct sockaddr_in addr;

  struct pollfd pollfds[POLL_NFDS];

  struct input user_input = { 0 }, server_input = { 0 };

  signal(SIGINT, termination_handler);
  signal(SIGTERM, termination_handler);

  if (argc != 3) {
    error(0, 0, "usage: %s hostname port", argv[0]);
    result = EXIT_FAILURE;
    goto main_none;
  }

  if(get_addr(&addr, argv[1], argv[2]) != 0) {
    result = -1;
    goto main_none;
  }

  serverfd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverfd < 0) {
    result = EXIT_FAILURE;
    error(0, errno, "could not open socket");
    goto main_none;
  }

  if (connect(serverfd, (struct sockaddr *) &addr, sizeof(addr)) != 0)  {
    error(0, errno, "could not connect to server");
    result = EXIT_FAILURE;
    goto main_serverfd;
  }

  printf("Connected to %s:%s\n", argv[1], argv[2]);
  fflush(stdout);

  set_nonblocking(STDIN_FILENO);
  set_nonblocking(serverfd);

  pollfds[0].fd = STDIN_FILENO;
  pollfds[0].events = POLLIN;
  pollfds[1].fd = serverfd;
  pollfds[1].events = POLLIN;
  pollfds[0].revents = pollfds[1].revents = 0;
  while (!finished) {
    poll(pollfds, POLL_NFDS, POLL_TIME);

    if (pollfds[0].revents & POLLIN) {
      int got = append_input(&user_input, STDIN_FILENO);
      if (got > 0) {
        if (write(serverfd, user_input.str, user_input.read) < 0) {
          result = -1;
          error(0, errno, "could not send message");
        }
        user_input.read = 0;
      } else if (got < 0) {
        result = -1;
        error(0, errno, "could not read from stdin");
        goto main_serverfd;
      }
      pollfds[0].revents = 0;
    }
    if (pollfds[1].revents & POLLIN) {
      int got = append_input(&server_input, serverfd);
      if (got > 0) {
        fputs(server_input.str, stdout);
        fflush(stdout);
        server_input.read = 0;
      } else if (got < 0) {
        result = -1;
        error(0, errno, "could not receive message");
        goto main_serverfd;
      }
      pollfds[1].revents = 0;
    }
  }

main_serverfd:
  close(serverfd);

main_none:
  free(user_input.str);
  free(server_input.str);

  return result;
}

void termination_handler(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    finished = 1;
  }

  signal(sig, termination_handler);
}

int get_addr(struct sockaddr_in *addr, const char *host, const char *port) {
  struct addrinfo *info;
  struct addrinfo hint;
  int result;

  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = hint.ai_flags = 0;

  result = getaddrinfo(host, port, &hint, &info);
  if (result == 0) {
    *addr = *((struct sockaddr_in *) info->ai_addr);
    freeaddrinfo(info);
  } else {
    error(0, 0, "could not resolve hostname: %s", gai_strerror(result));
  }

  return result;
}

int append_input(struct input *input, int fd) {
  int read_ = 0;

  if (input->str == NULL || input->capacity == 0) {
    input->capacity = DEFAULT_CAPACITY;
    input->str = (char *) malloc(input->capacity + 1);
    input->read = 0;
  }

  do {
    while (input->read + READ_SIZE >= input->capacity) {
      char *tmp;
      tmp = (char *) realloc(input->str, input->capacity * 2 + 1);
      if (tmp != NULL) {
        input->str = tmp;
        input->capacity *= 2;
      } else {
        return -1;
      }
    }
    read_ = read(fd, input->str + input->read, READ_SIZE);
    if (read_ > 0) {
      input->read += read_;
      input->str[input->read] = '\0';
    }
  } while (read_ > 0);
  if (read_ < 0 && errno != EAGAIN)
    return -1;
  if (read_ == 0) {
    finished = 1;
    return input->read;
  }

  return input->str[input->read - 1] == '\n' ? input->read : 0;
}

void set_nonblocking(int fd) {
      int flags = fcntl(fd, F_GETFL, 0);
          fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
