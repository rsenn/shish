<<<<<<< HEAD
#ifdef WIN32
=======
#ifdef _WIN32
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <io.h>
#else
#include <unistd.h>
#endif

#include "open.h"
#include "buffer.h"

int buffer_truncfile(buffer *b, const char *fn, char *y, unsigned long ylen)
{
  if((b->fd = open_trunc(fn)) == -1) return -1;
  b->p=0; b->n = 0;
  b->a=ylen; b->x=y;
  b->op = write;
  b->todo = NOTHING;
  return 0;
}
