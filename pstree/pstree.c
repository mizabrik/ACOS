#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <dirent.h>

void tree(int pid, unsigned level);
void print_name(const char *name, unsigned level);

int main() {
  tree(1, 0);

  return EXIT_SUCCESS;
}

void tree(int pid, unsigned level) {
  DIR *dir;
  struct dirent *entry;
  char path[256];
  char name[256];
  FILE *input;
  int child;

  sprintf(path, "/proc/%d/stat", pid);
  input = fopen(path, "r");
  fscanf(input, "%s", name);
  fscanf(input, "%s", name);
  print_name(name, level);
  fclose(input);

  sprintf(path, "/proc/%d/task", pid);
  dir = opendir(path);
  if (dir == NULL)
    return;

  while((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    sprintf(path, "/proc/%d/task/%s/children", pid, entry->d_name);
    input = fopen(path, "r");
    if (input == NULL)
      continue;

    while (fscanf(input, "%d", &child) != EOF)
      tree(child, level + 1);

    fclose(input);
  }
}

void print_name(const char *name, unsigned level) {
  unsigned i = 0;

  for (i = 0; i < level; ++i)
    putchar(' ');
  puts(name);
}
