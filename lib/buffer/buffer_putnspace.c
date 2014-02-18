#include <stdlib.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif /* HAVE_ALLOCA_H */
#include "str.h"
#include "byte.h"
#include "buffer.h"

int buffer_putnspace(buffer* b, int n) 
{
  char *space;
  if(n <= 0)
    return 0;
  space = alloca(n);
  byte_fill(space, n, ' ');
  return buffer_put(b,space,n);
}
