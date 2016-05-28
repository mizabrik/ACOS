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
#include <poll.h>

#include "common.h"
#include "life.h"
#include "udp_common.h"

struct client_data {
  unsigned y_begin;
  unsigned y_end;
  struct sockaddr_in addr;
  int ready;
};
typedef struct client_data client_data_t;

struct coordinator_args {
  life_t *life;
  life_t *tmp;
  unsigned n_steps;
  unsigned n_clients; 
  sem_t *ready; 
  sem_t *nexts;
};

void coordinator_thread(struct coordinator_args *args) {
  coordinator(args->life, args->tmp, args->n_steps, args->n_clients, args->ready, args->nexts);
}

int udper(int sockfd, life_t *life, life_t *tmp, unsigned steps, unsigned n_clients, sem_t *ready, sem_t *nexts);
size_t client_block_size(life_t *life, client_data_t *client);
ssize_t accept_client(int sockfd, unsigned id, unsigned width, unsigned height, struct sockaddr_in addr);
ssize_t send_map(int sockfd, unsigned id, life_t *life, client_data_t *client);
ssize_t send_borders(int sockfd, unsigned id, life_t *life, client_data_t *client);

int main(int argc, char* argv[]) {
  int rc;
  int n_clients, steps;
  int i;
  int port;

  if(argc < 4) {
    error(EXIT_FAILURE, 0, "usage: %s MAP PORT CLIENTS STEPS PORT", argv[0]);
  }

  n_clients = atoi(argv[2]);
  steps = atoi(argv[3]);
  port = atoi(argv[4]);
  
  FILE *file = fopen(argv[1], "r");
  life_t life;
  life.field = NULL;
  rc = life_read(&life, file);
  if(rc < 0) {
    error(EXIT_FAILURE, 0, "Can't read from file\n");
  }

  life_print(&life, stdout);
  life_t tmp;
  tmp.field = NULL;
  life_new(&tmp, life.width, life.height);

  sem_t ready;
  sem_t nexts[n_clients];
  rc = sem_init(&ready, 0, 0);
  if(rc != 0) {
    error(EXIT_FAILURE, errno, "Problems with syncronisation");
  }
  for(i = 0; i < n_clients; i++) {
    rc = sem_init(&nexts[i], 0, 0);
    if(rc != 0) {
      error(EXIT_FAILURE, errno, "Problems with syncronisation");
    }
  }

  struct coordinator_args args;
  args.life = &life;
  args.tmp = &tmp;
  args.n_steps = steps;
  args.n_clients = n_clients;
  args.ready = &ready;
  args.nexts = nexts;
  pthread_t coordinator_pt;
  pthread_create(&coordinator_pt, NULL, (void * (*)(void*))coordinator_thread, &args);

  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));

  udper(sockfd, &life, &tmp, steps, n_clients, &ready, nexts);

  life_print(&life, stdout);

  life_destroy(&life);
  life_destroy(&tmp);

  for(i = 0; i < n_clients; ++i)
    sem_destroy(&nexts[i]);

  return EXIT_SUCCESS;
}

int udper(int sockfd, life_t *life, life_t *tmp, unsigned steps, unsigned n_clients, sem_t *ready, sem_t *nexts) {
  client_data_t *clients;
  unsigned i;
  struct msg *buffer;
  size_t buffer_size = 0;
  unsigned n_connected = 0;
  unsigned step = 0;
  unsigned n_ready = 0;
  int finished = 0;
  struct pollfd fds[1];
  int rc;
  int state = CONNECT;

  clients = (client_data_t *) malloc(sizeof(client_data_t) * n_clients);
  int block_size = life->height / n_clients;
  for (i = 0; i < n_clients; ++i) {
    clients[i].y_begin = i * block_size;
    clients[i].y_end = (i + 1) * block_size;
    clients[i].ready = 0;
  }
  clients[n_clients - 1].y_end = life->height;

  buffer_size = (clients[n_clients - 1].y_end - clients[n_clients -1].y_begin + 2) * life->width;
  buffer_size += sizeof(struct msg_info);
  buffer = (struct msg *) malloc(buffer_size);

  fds[0].fd = sockfd;
  fds[0].events = POLLIN;
  int timeout = 1000;

  while (!finished) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    unsigned id;

    rc = poll(fds, 1, timeout);
    if(rc == 0) {
      if(state != CONNECT) {
        error(EXIT_FAILURE, 0, "Too long waiting");
      }
    }
    recvfrom(sockfd, buffer, buffer_size, 0, (struct sockaddr *) &addr, &addr_len);
    id = buffer->id;

    switch (buffer->type) {
      case MSG_INIT:
        if (n_connected < n_clients) {
          int flag = 1;
          for(i = 0; i < n_connected; ++i) {
            if(clients[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr && clients[i].addr.sin_port == addr.sin_port) {
              accept_client(sockfd, n_connected, life->width, life->height, addr);
              flag = 0;
              break;
            }
          }
          if(flag) {
            clients[n_connected].addr = addr;
            accept_client(sockfd, n_connected, life->width, life->height, addr);
            ++n_connected;
          }
        }
        break;

      case MSG_MAP_REQUEST:
        if (id < n_connected) {
          state = SEND_MAP;
          send_map(sockfd, id, life, &clients[id]);
          send_borders(sockfd, id, life, &clients[id]);
        }
        break;

      case MSG_MAP_TRANSFER:
        if (id < n_connected && clients[id].ready == 0) {
          state = GET_MAP;
          memcpy(tmp->field + cell_id(life, 0, clients[id].y_begin),
                 ((struct msg_map_transfer *) buffer)->map,
                 client_block_size(life, &clients[id]));
          clients[id].ready = 1;
          sem_post(ready);
          ++n_ready;
        }
        break;

      default:
        break;
    }

    if (n_ready == n_clients) {
      ++step;
      if (step == steps) {
        finished = 1;
      } else {
        state = SEND_UPDATES;
        for (i = 0; i < n_clients; ++i) {
          sem_wait(&nexts[i]);
          send_borders(sockfd, i, life, &clients[i]);
        }
      }
    }
  }

  free(clients);

  return 0;
}

size_t client_block_size(life_t *life, client_data_t *client) {
  return (client->y_end - client->y_begin) * life->width;
}

ssize_t accept_client(int sockfd, unsigned id, unsigned width, unsigned height, struct sockaddr_in addr) {
  struct msg_info info;

  info.msg.type = MSG_INFO;
  info.msg.id = id;
  info.width = width;
  info.height = height;

  return sendto(sockfd, &info, sizeof(info), 0, (struct sockaddr *) &addr, sizeof(addr));
}

ssize_t send_map(int sockfd, unsigned id, life_t *life, client_data_t *client) {
  struct msg_map_transfer *map;
  size_t size, map_size;

  map_size = client_block_size(life, client);
  size = sizeof(*map) + map_size;
  map = (struct msg_map_transfer *) malloc(size);

  map->msg.type = MSG_MAP_TRANSFER;
  map->msg.id = id;
  memcpy(map->map, life->field + cell_id(life, 0, client->y_begin), map_size);

  return sendto(sockfd, map, size, 0, (struct sockaddr *) &client->addr, sizeof(client->addr));
}

ssize_t send_borders(int sockfd, unsigned id, life_t *life, client_data_t *client) {
  struct msg_map_borders *borders;
  size_t size, borders_size;

  borders_size = 2 * life->width;
  size = sizeof(*borders) + borders_size;
  borders = (struct msg_map_borders *) malloc(size);

  borders->msg.type = MSG_MAP_BORDERS;
  borders->msg.id = id;
  memcpy(borders->borders, life->field + cell_id(life, 0, client->y_begin - 1), life->width);
  memcpy(borders->borders + life->width, life->field + cell_id(life, 0, client->y_end), life->width);

  return sendto(sockfd, borders, size, 0, (struct sockaddr *) &client->addr, sizeof(client->addr));
}
