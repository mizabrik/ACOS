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

enum STATE {
  STATE_CONNECT,
  STATE_INIT,
  SEND_MAP,
  GET_MAP,
  SEND_UPDATES,
};

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
  int *finished;
};

void coordinator_thread(struct coordinator_args *args) {
  coordinator(args->life, args->tmp, args->n_steps, args->n_clients, args->ready, args->nexts, args->finished);
}

int udper(int sockfd, life_t *life, life_t *tmp, unsigned steps, unsigned n_clients);
size_t client_block_size(life_t *life, client_data_t *client);
ssize_t accept_client(int sockfd, unsigned id, unsigned width, unsigned height, unsigned steps, struct sockaddr_in addr);
ssize_t send_part(int sockfd, unsigned id, life_t *life, client_data_t *client);
ssize_t send_borders(int sockfd, unsigned id, life_t *life, client_data_t *client);

int main(int argc, char* argv[]) {
  int rc;
  int n_clients, steps;
  int port;

  if(argc < 4) {
    error(EXIT_FAILURE, 0, "usage: %s MAP CLIENTS STEPS PORT", argv[0]);
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
  if (n_clients > life.height)
    error(-1, 0, "too many clients for map of height %d", life.height);

  life_t tmp;
  tmp.field = NULL;
  life_new(&tmp, life.width, life.height);

  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)))
    error(-1, errno, "could not bind socket");

  udper(sockfd, &life, &tmp, steps, n_clients);

  life_print(&life, stdout);

  life_destroy(&life);
  life_destroy(&tmp);

  return EXIT_SUCCESS;
}

int udper(int sockfd, life_t *life, life_t *tmp, unsigned steps, unsigned n_clients) {
  client_data_t *clients;
  unsigned i;
  struct msg *buffer;
  size_t buffer_size = 0;
  unsigned n_connected = 0;
  unsigned n_ready = 0;
  int rc;
  int state = STATE_CONNECT;
  int waiting = 0;

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

  while (n_ready < n_clients) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    unsigned id;

    //rc = try_receive(sockfd, buffer, buffer_size, (struct sockaddr *) &addr, &addr_len, TIMEOUT);
    rc = try_receive(sockfd, buffer, buffer_size, (struct sockaddr *) &addr, &addr_len, -1);
    if (rc < 0) {
      ++waiting;
      if (waiting >= MAX_TRIES && state != STATE_CONNECT)
        return -1;
      continue;
    }

    id = buffer->id;
    switch (buffer->type) {
      case MSG_INIT:
        if (n_connected < n_clients) {
          int new_id = -1;
          for(i = 0; i < n_connected; ++i) {
            if(clients[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr && clients[i].addr.sin_port == addr.sin_port) {
              new_id = i; 
              break;
            }
          }
          if (new_id < 0) {
            new_id = n_connected;
            clients[new_id].addr = addr;
            ++n_connected;
            if (n_connected == n_clients) {
              state = STATE_INIT;
              waiting = 0;
            }
          }
          int height = clients[new_id].y_end - clients[new_id].y_begin;
          accept_client(sockfd, new_id, life->width, height, steps, addr);
        }
        break;

      case MSG_MAP_REQUEST:
        if (id < n_connected) {
          state = SEND_MAP;
          send_part(sockfd, id, life, &clients[id]);
          send_borders(sockfd, id, life, &clients[id]);

          ++n_ready;
        }
        break;

      default:
        break;
    }
  }

  struct msg_neighbours neighbours;
  neighbours.msg.type = MSG_NEIGHBOURS_INFO; 
  neighbours.left.id = n_clients - 1;
  neighbours.left.addr = clients[n_clients - 1].addr;
  for (unsigned id = 0; id < n_clients - 1; ++id) {
    neighbours.right.id = id + 1;
    neighbours.right.addr = clients[id + 1].addr;

    sendto(sockfd, &neighbours, sizeof(neighbours), 0,
           (struct sockaddr *) &clients[id].addr, sizeof(clients[id].addr));

    neighbours.left.id = id;
    neighbours.left.addr = clients[id].addr;
  }
  neighbours.right.id = 0;
  neighbours.right.addr = clients[0].addr;
  sendto(sockfd, &neighbours, sizeof(neighbours), 0,
         (struct sockaddr *) &clients[n_clients - 1].addr, sizeof(clients[n_clients].addr));

  for (unsigned i = 0; i < n_clients; ++i) {
    try_receive(sockfd, buffer, buffer_size, NULL, NULL, TIMEOUT);

    unsigned id = buffer->id;
    memcpy(life->field + cell_id(life, 0, clients[id].y_begin),
           ((struct msg_map_transfer *) buffer)->map,
           client_block_size(life, &clients[id]));
  }

  free(clients);

  return 0;
}

size_t client_block_size(life_t *life, client_data_t *client) {
  return (client->y_end - client->y_begin) * life->width;
}

ssize_t accept_client(int sockfd, unsigned id, unsigned width, unsigned height, unsigned steps, struct sockaddr_in addr) {
  struct msg_info info;

  info.msg.type = MSG_INFO;
  info.msg.id = id;
  info.width = width;
  info.height = height;
  info.steps = steps;

  return sendto(sockfd, &info, sizeof(info), 0, (struct sockaddr *) &addr, sizeof(addr));
}

ssize_t send_part(int sockfd, unsigned id, life_t *life, client_data_t *client) {
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
  struct msg_map_border *border;
  size_t size, border_size;

  border_size = life->width;
  size = sizeof(*border) + border_size;
  border = (struct msg_map_border *) malloc(size);

  border->msg.type = MSG_MAP_BORDER;
  border->msg.id = id;

  border->source = SIDE_RIGHT;
  memcpy(border->border, life->field + cell_id(life, 0, client->y_end), life->width);
  sendto(sockfd, border, size, 0, (struct sockaddr *) &client->addr, sizeof(client->addr));
  
  border->source = SIDE_LEFT;
  memcpy(border->border, life->field + cell_id(life, 0, client->y_begin - 1), life->width);
  sendto(sockfd, border, size, 0, (struct sockaddr *) &client->addr, sizeof(client->addr));

  return 0;
}
