#ifndef UDP_COMMON_H_
#define UDP_COMMON_H_

enum MSG_TYPES {
  MSG_INIT,
  MSG_INFO,
  MSG_MAP_REQUEST,
  MSG_MAP_TRANSFER,
  MSG_MAP_BORDERS,
};

enum STATE {
  CONNECT,
  SEND_MAP,
  GET_MAP,
  SEND_UPDATES,
};

struct msg {
  int type;
  unsigned id;
};

struct msg_info {
  struct msg msg;
  unsigned width;
  unsigned height;
};

struct msg_map_request {
  struct msg msg;
};

struct msg_map_transfer {
  struct msg msg;
  char map[];
};

struct msg_map_borders {
  struct msg msg;
  char borders[];
};

#endif /* UDP_COMMON_H_ */
