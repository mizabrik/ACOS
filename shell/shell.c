#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "args.h"

#define GETTOK(str) strtok(str, " \n")
#define MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int exec(const char *path, char **args, int in, int out) {
  int stat_loc;
  pid_t pid = fork();

  if (pid == 0) {
    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);
    execvp(path, args);
    exit(0);
  } else {
    waitpid(pid, &stat_loc, 0);
  }

  return 0;
}

int execute(char *command) {
  const char *path;
  args_t *args;
  const char *token;
  int in = STDIN_FILENO, out = STDOUT_FILENO;
 
  path = NULL; 
  args = (args_t *) malloc(sizeof(args_t));
  args_new(args);
  token = GETTOK(command);
  while (token != NULL) {
    if (strcmp(token, "<") == 0) {
      token = GETTOK(NULL);
      in = open(token, O_RDONLY);
    } else if (strcmp(token, ">") == 0) {
      token = GETTOK(NULL);
      out = open(token, O_WRONLY | O_CREAT | O_TRUNC, MODE);
    } else {
      if (path == NULL)
        path = token;

      args_add(args, token);
    }
     
    token = GETTOK(NULL);
  }

  if (path != NULL) {
    exec(path, args->argv, in, out);
    if (in != STDIN_FILENO)
      close(in);
    if (out != STDOUT_FILENO)
      close(out);
  }

  args_exterminate(args);

  return 0;
}

int main() {
  char *line;
  size_t size = 0;
  ssize_t n_read = 0;

  do {
    fputs("$ ", stdout);
    n_read = getline(&line, &size, stdin);
    execute(line);
  } while(n_read > 0);

  return EXIT_SUCCESS;
}
