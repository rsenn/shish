

#include <stdlib.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif /* HAVE_ALLOCA_H */

#include "../buffer.h"
#include "../byte.h"
#include "../str.h"

int
buffer_putnspace(buffer* b, int n) {
  if(n > 0) {
    int ret;
    char* space =
#ifdef HAVE_ALLOCA
        alloca(n);
#else
        malloc(n);
#endif
    byte_fill(space, n, ' ');
    ret = buffer_put(b, space, n);
#ifndef HAVE_ALLOCA
    free(space);
#endif
    return ret;
  }
  return 0;
}
