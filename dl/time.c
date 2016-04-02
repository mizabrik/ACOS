#include <stdlib.h>
#include <time.h>

char *speak() {
  time_t timer;
  struct tm *time_info;
  char *speech = (char *) malloc(80);

  time(&timer);
  time_info = localtime(&timer);
  strftime(speech, 80, "It's %H:%M o'clock", time_info);

  return speech;
}
