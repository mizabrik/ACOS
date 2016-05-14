#include <error.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "array.h"
#include "command.h"
#include "get_line.h"

int exec(const char *path, char **args, int in, int out,
         int *close_fds, int close_count) {
  pid_t pid = fork();

  if (pid == 0) {
    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);

    while (close_count) {
      close(*close_fds);
      ++close_fds;
      --close_count;
    }

    execvp(path, args);
    exit(EXIT_SUCCESS);
  }

  return pid;
}

int exec_detached(const char *path, char **args, int in, int out,
                  int *close_fds, int close_count) {
  int res;
  pid_t pid = fork();

  if (pid == 0) {
    int res = exec(path, args, in, out, close_fds, close_count);
    exit(res);
  } else if (pid > 0) {
    int stat_loc;
    res = waitpid(pid, &stat_loc, 0);
  } else {
    res = -1;
  }
  
  return res;
}

int execute_command(command_t command) {
  int pid;
  int stat_loc;
  int pipe_count;
  int close_fds[4];
  int close_count = 0;
  int pipe_fds[2];
  int i;
  int in, out;
  char **argv;

  if (command.output != STDOUT_FILENO)
    close_fds[close_count++] = command.output;
  if (command.input != STDIN_FILENO)
    close_fds[close_count++] = command.input;

  in = command.input;
  pipe_count = command.programs.size - 1;
  for (i = 0; i < pipe_count; ++i) {
    pipe(pipe_fds);
    out = pipe_fds[1];
    close_fds[close_count++] = pipe_fds[0];
    close_fds[close_count++] = pipe_fds[1];

    argv = (char **) ((array_t *) command.programs.data[i])->data;
    exec_detached(argv[0], argv, in, out, close_fds, close_count);

    close(out);
    --close_count;
    if (i || in != STDIN_FILENO) {
      close(in);
      --close_count;
      close_fds[close_count - 1] = close_fds[close_count]; /* move input fd */
    }
    in = pipe_fds[0];
  }
  out = command.output;
  argv = (char **) ((array_t *) command.programs.data[i])->data;
  pid = exec(argv[0], argv, in, out, close_fds, close_count);
  if (out != STDOUT_FILENO)
    close(out);
  waitpid(pid, &stat_loc, 0);

  return 0;
}

int main() {
  command_t command;
  do {
    char *line;
    int size;

    fputs("$ ", stdout);
    size = get_line(&line, stdin);
    if (size && !parse(&command, line))
      execute_command(command);
    free(line);
  } while(!feof(stdin));
  fputc('\n', stdout);

  return EXIT_SUCCESS;
}
