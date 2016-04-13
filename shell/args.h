struct args {
  char **argv;
  int argc;

  int capacity;
};
typedef struct args args_t;

void args_new(args_t *args);
void args_add(args_t *args, const char *arg);
void args_exterminate(args_t *args);
