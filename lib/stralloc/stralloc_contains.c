#include "../stralloc.h"

int
stralloc_contains(const stralloc* sa, const char* what) {
  return stralloc_finds(sa, what) != sa->len;
}
