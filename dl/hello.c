#include <stdlib.h>
#include <string.h>

#define STRING "Hello, world!"

char* speak() {
  char *speech = (char *) malloc(strlen(STRING));

  strcpy(speech, STRING);

  return speech;
}
