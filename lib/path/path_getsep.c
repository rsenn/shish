#include "internal.h"

int
path_getsep(const char* path) {
  while(*path) {
    if(SEP(*path))
      return *path;

    ++path;
  }

  return '\0';
}
