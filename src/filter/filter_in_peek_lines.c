#include "../filter.h"
#include "../../lib/byte.h"

/* zero-copy prefix holding at most *lines newlines */
ssize_t
filter_in_peek_lines(struct filter_in* in, const char** p, unsigned long* lines) {
  const char* w;
  size_t n, i = 0;
  ssize_t r = filter_in_peek(in, p);

  if(r <= 0)
    return r;

  w = *p;
  n = (size_t)r;

  while(*lines && i < n) {
    size_t k = byte_chr(w + i, n - i, '\n');

    if(i + k >= n) {
      i = n;
      break;
    }

    i += k + 1;
    (*lines)--;
  }

  return (ssize_t)i;
}
