#ifndef UDP_COMMON_H_
#define UDP_COMMON_H_

#include <sys/socket.h>
#include <netinet/ip.h>

#define MAX_TRIES 3
#define TIMEOUT 5

enum MSG_TYPES {
  MSG_INIT,
  MSG_INFO,
  MSG_MAP_REQUEST,
  MSG_MAP_TRANSFER,
  MSG_MAP_BORDER,
  MSG_FINISH,
  MSG_NEIGHBOURS_INFO
};

struct msg {
  int type;
  unsigned id;
};

struct msg_info {
  struct msg msg;
  unsigned width;
  unsigned height;
  unsigned steps;
};

struct msg_map_request {
  struct msg msg;
};

struct msg_map_transfer {
  struct msg msg;
  char map[];
};

enum SIDE {
  SIDE_LEFT,
  SIDE_RIGHT
};

struct msg_map_border {
  struct msg msg;
  enum SIDE source;
  char border[];
};

struct neighour {
  unsigned id;
  struct sockaddr_in addr;
} left, right;

struct msg_neighbours {
  struct msg msg;
  struct neighour left;
  struct neighour right;
};

int try_receive(int sockfd, void *buffer, size_t size, struct sockaddr *addr, socklen_t *addr_size, int timeout);

#endif /* UDP_COMMON_H_ */
