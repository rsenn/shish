#include "../str.h"

int
str_startb(const char* a, const char* x, size_t len) {
  size_t i;
  for(i = 0;; i++) {
    if(i == len)
      return 1;
    if(a[i] != x[i])
      break;
  }
  return 0;
}
