#include "command.h"

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "array.h"

#define GETTOK(str) strtok(str, " \n")

#define PARSE_ERROR(code, text) \
  { \
    error(0, code, text); \
    err = -1; \
    break; \
  } \

#define MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

char * copy_string(const char *source) {
  char * destination = (char *) malloc(strlen(source));
  if (destination) {
    strcpy(destination, source);
  }
  return destination;
}

int parse(command_t *command, const char *input) {
  char *path;
  char *token;
  int fd;
  char *buffer;
  int err;
  array_t *args;

  err = 0;
  path = NULL;
  array_new(&command->programs);
  command->input = STDIN_FILENO;
  command->output = STDOUT_FILENO;
  buffer = copy_string(input);
  args = (array_t *) malloc(sizeof(array_t));
  array_new(args);
  token = GETTOK(buffer);
  while (token != NULL) {
    if (strcmp(token, "<") == 0) {
      token = GETTOK(NULL);
      if (token == NULL) {
        PARSE_ERROR(0, "no redirection destination");
      }
      if (command->input != STDIN_FILENO) {
        error(0, 0, "ingnoring '< %s', redirection is already specified",
              token);
      } else if ((fd = open(token, O_RDONLY)) != -1) {
        command->input = fd;
      } else {
        PARSE_ERROR(errno, "could not open input file");
      }
    } else if (strcmp(token, ">") == 0) {
      token = GETTOK(NULL);
      if (token == NULL) {
        PARSE_ERROR(0, "no redirection destination");
      }
      if (command->output != STDOUT_FILENO) {
        error(0, 0, "ingnoring '> %s', redirection is already specified",
              token);
      } else if ((fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, MODE)) != -1) {
        command->output = fd;
      } else {
        PARSE_ERROR(errno, "could not open output file");
      }
    } else if (strcmp(token, "|") == 0) {
      array_add(&command->programs, args, sizeof(array_t *));
      args = (array_t *) malloc(sizeof(array_t));
      array_new(args);
      path = NULL;
    } else {
      if (path == NULL)
        path = token;
      array_add(args, token, strlen(token));
    }
     
    token = GETTOK(NULL);
  }
  array_add(&command->programs, args, sizeof(array_t *));

  if (path == NULL) {
    error(0, 0, "no pipe destination specified");
    err = -1;
  }

  if (err) {
    int i;
    for (i = 0; i < command->programs.size; ++i) {
      array_delete((array_t *) command->programs.data[i]);
    }
    array_delete(&command->programs);
  }

  return err;
}
