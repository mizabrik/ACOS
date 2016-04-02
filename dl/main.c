#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>

#define PLUGINS_DIR "plugins"
#define PLUGINS_DIR_LEN 7

void run_plugin(const char *name);

int main() {
  DIR *plugins_dir;
  struct dirent *plugin;
  unsigned plugin_cnt;

  plugins_dir = opendir(PLUGINS_DIR);

  plugin_cnt = 0;
  while ((plugin = readdir(plugins_dir)) > 0) {
    if (plugin->d_name[0] != '.')
      run_plugin(plugin->d_name);
  }

  return 0;
}

void run_plugin(const char *name) {
  char *path;
  size_t name_length;
  void *handle;

  name_length = strlen(name);
  path = (char *) malloc(name_length + 1 + PLUGINS_DIR_LEN + 1);
  strcpy(path, PLUGINS_DIR);
  path[PLUGINS_DIR_LEN] = '/';
  strcpy(path + PLUGINS_DIR_LEN + 1, name);
  
  handle = dlopen(path, RTLD_LAZY);
  if (handle != NULL) {
    char *speech;
    char *(*speak)() = (char * (*) ()) dlsym(handle, "speak");
    if (speak != NULL) {
      speech = speak();
      printf("Plugin '%s' says: '%s'.\n", name, speech);
    } else {
      printf("Plugin '%s' can't speak :(\n", name);
    }

    dlclose(handle);
  } else {
    fprintf(stderr, "Could not load plugin '%s'.\n", name);
  }
  free(path);
}
