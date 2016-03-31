#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <dirent.h>

char path[PATH_MAX];

void tree(char *base);

int main(int argc, char **argv) {
  char *base;

  if (argc > 1) {
    strcpy(path, argv[1]);
  } else {
    path[0] = '.';
  }

  base = path;
  while (*base != '\0')
    ++base;
  assert(base != path);

  tree(base);

  return EXIT_SUCCESS;
}

void tree(char *base) {
  DIR *dir;
  struct dirent *entry;

  dir = opendir(path);
  if (dir == NULL) {
    printf("Could not open %s\n", path);
    return;
  }

  while((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    printf("%s/%s\n", path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      char *new_base;

      new_base = base;
      if (*(base - 1) != '/') {
	*new_base = '/';
	++new_base;
      }

      strcpy(new_base, entry->d_name);
      while (*new_base != '\0')
	++new_base;

      tree(new_base);

      *base = '\0';
    }
  }
}
