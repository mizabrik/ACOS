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

int init(int sockfd, struct sockaddr_in *server, unsigned *id, unsigned *width, unsigned *height, unsigned *steps);
int get_neighbours(int sockfd);
int get_field(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life);
int get_borders(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life);
int send_borders(int sockfd, unsigned id, life_t *life);
int send_map(int sockfd, unsigned id, struct sockaddr_in *server, life_t *life);

struct neighour left, right;

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

  unsigned id, width, height, steps;
  if (init(sockfd, &server, &id, &width, &height, &steps) < 0)
    error(-1, 0, "could not connect to server");

  life_new(&life, width, height + 2);
  life_new(&tmp, width, height + 2);

  printf("Connected\n");
  get_field(sockfd, id, &server, &life);
  printf("Got map\n");

  get_neighbours(sockfd);
  printf("Got neighbours\n");

  sem_t ready, next;
  sem_init(&ready, 0, 0);
  sem_init(&next, 0, 0);

  int finished = 0;
  worker_data_t data;
  data.new = &tmp;
  data.old = &life;
  data.y_begin = 2;
  data.y_end = life.height;
  data.next = &next;
  data.ready = &ready;
  data.finished = &finished;
  pthread_t worker_pt;
  pthread_create(&worker_pt, NULL, (void * (*)(void*))worker, &data);

  int ret;
  for (unsigned step = 0; step < steps; ++step) {
    sem_post(&next);
    sem_wait(&ready);

    swap_lifes(&life, &tmp);
    send_borders(sockfd, id, &life);
    ret = get_borders(sockfd, id, &server, &life);
    printf("finished step\n");
  }

  send_map(sockfd, id, &server, &life);

  finished = 1;
  sem_post(&next);

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

int init(int sockfd, struct sockaddr_in *server, unsigned *id, unsigned *width, unsigned *height, unsigned *steps) {
  struct msg msg;
  struct msg_info info;
  int try;
  int rc = -1;

  msg.type = MSG_INIT;
  msg.id = 0;

  for (try = 0; try < MAX_TRIES && rc < 0; ++try) {
    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *) server, sizeof(*server));
    rc = try_receive(sockfd, &info, sizeof(info), NULL, NULL, TIMEOUT);
  }
  if (rc > 0) {
    *id = info.msg.id;
    *width = info.width;
    *height = info.height;
    *steps = info.steps;
  }

  return rc;
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
  struct msg_map_border *border;
  size_t borders_size = life->width;
  size_t size = sizeof(*border) + borders_size;
  border = (struct msg_map_border *) malloc(size);

  for (int i = 0; i < 2; ++i) {
    char *dest;

    recvfrom(sockfd, border, size, 0, NULL, NULL);
    switch (border->source) {
      case SIDE_LEFT:
        dest = life->field + life->width;
        break;

      case SIDE_RIGHT:
        dest = life->field;
        break;

      default:
        return -1;
        break;
    }
    memcpy(dest, border->border, borders_size);
  }

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

int get_neighbours(int sockfd) {
  struct msg_neighbours neighbours;
  
  recvfrom(sockfd, &neighbours, sizeof(neighbours), 0, NULL, NULL);

  left = neighbours.left;
  right = neighbours.right;
}

int send_borders(int sockfd, unsigned id, life_t *life) {
  struct msg_map_border *border;
  size_t size, borders_size;

  borders_size = life->width;
  size = sizeof(*border) + borders_size;
  border = (struct msg_map_border *) malloc(size);

  border->msg.type = MSG_MAP_BORDER;
  border->msg.id = id;

  border->source = SIDE_RIGHT;
  memcpy(border->border, life->field + life->width * 2, life->width);
  sendto(sockfd, border, size, 0, (struct sockaddr *) &left.addr, sizeof(left.addr));

  border->source = SIDE_LEFT;
  memcpy(border->border, life->field + cell_id(life, 0, life->height - 1), life->width);
  sendto(sockfd, border, size, 0, (struct sockaddr *) &right.addr, sizeof(right.addr));

  return 0;
}
