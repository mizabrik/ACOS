#include <error.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "args.h"
#include "get_line.h"

#define GETTOK(str) strtok(str, " \n")
#define MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define EXECUTE_ERROR(code, text) \
  { \
    error(0, code, text); \
    path = NULL; \
    break; \
  } \

int exec(const char *path, char **args, int in, int out) {
  pid_t pid = fork();

  if (pid == 0) {
    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);
    execvp(path, args);
    exit(EXIT_SUCCESS);
  }

  return pid;
}

int exec_detached(const char *path, char **args, int in, int out) {
  int res;
  pid_t pid = fork();

  if (pid == 0) {
    int res = exec(path, args, in, out);
    exit(res);
  } else if (pid > 0) {
    int stat_loc;
    res = waitpid(pid, &stat_loc, 0);
  } else {
    res = -1;
  }
  
  return res;
}

int execute(char *command) {
  const char *path;
  args_t *args;
  const char *token;
  int stat_loc;
  int in = STDIN_FILENO, out = STDOUT_FILENO;
  int fd;

  path = NULL; 
  args = (args_t *) malloc(sizeof(args_t));
  args_new(args);
  token = GETTOK(command);
  while (token != NULL) {
    if (strcmp(token, "<") == 0) {
      token = GETTOK(NULL);
      if (token == NULL)
        EXECUTE_ERROR(0, "no redirection destination");
      if ((fd = open(token, O_RDONLY)) != -1)
        in = fd;
      else
        EXECUTE_ERROR(errno, "could not open input file");
    } else if (strcmp(token, ">") == 0) {
      token = GETTOK(NULL);
      if (token == NULL)
        EXECUTE_ERROR(0, "no redirection destination");
      if ((fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, MODE)) != -1)
        out = fd;
      else
        EXECUTE_ERROR(errno, "could not open output file");
    } else if (strcmp(token, "|") == 0) {
      int pipe_fds[2];
      pipe(pipe_fds);
      if (out != STDOUT_FILENO)
        close(out);
      out = pipe_fds[1];
      exec_detached(path, args->argv, in, out);

      close(out);
      out = STDOUT_FILENO;
      if (in != STDIN_FILENO)
        close(in);
      in = pipe_fds[0];

      path = NULL;
      args_exterminate(args);
      args_new(args);
    } else {
      if (path == NULL)
        path = token;

      args_add(args, token);
    }
     
    token = GETTOK(NULL);
  }

  if (path != NULL) {
    int pid = exec(path, args->argv, in, out);
    waitpid(pid, &stat_loc, 0);
  }
  if (in != STDIN_FILENO)
    close(in);
  if (out != STDOUT_FILENO)
    close(out);

  args_exterminate(args);
  free(args);

  return 0;
}

int main() {
  do {
    char *line;
    int size;

    fputs("$ ", stdout);
    size = get_line(&line, stdin);
    execute(line);
    free(line);
  } while(!feof(stdin));
  fputc('\n', stdout);

  return EXIT_SUCCESS;
}
