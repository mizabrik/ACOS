#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "common.h"
#include "life.h"
#include "udp_common.h"

int get_addr(struct sockaddr_in *addr, const char *host, const char *port);

int init(int sockfd, struct sockaddr_in *server, unsigned *id, unsigned *width, unsigned *height);
int get_field(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life);
int get_borders(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life);
int send_map(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life);

int main(int argc, char **argv) {
  struct sockaddr_in addr;
  struct sockaddr_in server;

  if (argc != 3) {
    error(-1, 0, "usage: %s HOST PORT", argv[0]);
  }

  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));

  get_addr(&server, argv[1], argv[2]);

  life_t life, tmp;

  unsigned id, width, height;
  init(sockfd, &server, &id, &width, &height);

  life_new(&life, width, height + 2);
  life_new(&tmp, width, height + 2);

  get_field(sockfd, id, &server, &life);

  sem_t ready, next;
  sem_init(&ready, 0, 0);
  sem_init(&next, 0, 0);

  worker_data_t data;
  data.new = &tmp;
  data.old = &life;
  data.y_begin = 2;
  data.y_end = life.height;
  data.steps = -1;
  data.next = &next;
  data.ready = &ready;
  pthread_t worker_pt;
  pthread_create(&worker_pt, NULL, (void * (*)(void*))worker, &data);

  int ret;
  do {
    sem_post(&next);
    sem_wait(&ready);

    swap_lifes(&life, &tmp);
    send_map(sockfd, id, &server, &life);
    ret = get_borders(sockfd, id, &server, &life);
  } while (!ret);

  sem_destroy(&next);
  sem_destroy(&ready);
  life_destroy(&life);
  life_destroy(&tmp);

  return 0;
}

int get_addr(struct sockaddr_in *addr, const char *host, const char *port) {
  struct addrinfo *info;
  struct addrinfo hint;
  int result;

  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_DGRAM;
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

int init(int sockfd, struct sockaddr_in *server, unsigned *id, unsigned *width, unsigned *height) {
  struct msg msg;
  struct msg_info info;

  msg.type = MSG_INIT;
  msg.id = 0;
  sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *) server, sizeof(*server));

  recvfrom(sockfd, &info, sizeof(info), 0, NULL, NULL);
  *id = info.msg.id;
  *width = info.width;
  *height = info.height;

  return 0;
}

int get_field(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life) {
  struct msg request;
  struct msg_map_transfer *map;
  size_t map_size = life->width * (life->height - 2);
  size_t size = sizeof(*map) + map_size;
  map = (struct msg_map_transfer *) malloc(size);

  request.id = id;
  request.type = MSG_MAP_REQUEST;
  sendto(sockfd, &request, sizeof(request), 0, (struct sockaddr *) server, sizeof(*server));

  recvfrom(sockfd, map, size, 0, NULL, NULL);
  memcpy(life->field + 2 * life->width, map->map, map_size);

  get_borders(sockfd, id, server, life);

  return 0;
}

int get_borders(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life) {
  struct msg_map_borders *borders;
  size_t borders_size = 2 * life->width;
  size_t size = sizeof(*borders) + borders_size;
  borders = (struct msg_map_borders *) malloc(size);

  recvfrom(sockfd, borders, size, 0, NULL, NULL);
  memcpy(life->field, borders->borders, borders_size);

  return 0;
}

int send_map(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life) {
  struct msg_map_transfer *map;
  size_t map_size = life->width * (life->height - 2);
  size_t size = sizeof(*map) + map_size;
  map = (struct msg_map_transfer *) malloc(size);

  map->msg.type = MSG_MAP_TRANSFER;
  map->msg.id = id;
  memcpy(map->map, life->field + life->width * 2, map_size);

  sendto(sockfd, map, size, 0, (struct sockaddr *) server, sizeof(*server));

  return 0;
}
