#include "../byte.h"
#include "../stralloc.h"

size_t
stralloc_count(const stralloc* sa, char c) {
  return byte_count(sa->s, sa->len, c);
}
