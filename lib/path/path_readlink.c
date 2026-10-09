#include "../windoze.h"
#include "../unix.h"
#include "../path_internal.h"
#include "../str.h"
#include "../stralloc.h"

/* read the link into a stralloc, NUL-terminated; returns the length of the
 * target, or -1 with sa->len == 0 on error (errno from readlink())
 * ----------------------------------------------------------------------- */
int
path_readlink(const char* path, stralloc* sa) {
  /* most targets are short: start small and double */
  size_t n = 64;
  ssize_t sz;

  sa->len = 0;

  for(;;) {
    if(!stralloc_ready(sa, n))
      return -1;

    if((sz = readlink(path, sa->s, n)) < 0)
      return -1;

    /* a full buffer may have truncated the target */
    if((size_t)sz < n)
      break;

    n <<= 1;
  }

  sa->len = sz;
  stralloc_nul(sa);
  return (int)sz;
}
