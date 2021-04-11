#include "../path_internal.h"

int
path_getsep(const char* path) {
  while(*path) {
    if(path_issep(*path))
      return *path;
    ++path;
  }
  return '\0';
}
