#include <shell.h>
#include <sys/mman.h>
#include "buffer.h"

void buffer_free(buffer *b)
{
  switch (b->todo)
  {
    case FREE: shell_free(b->x); break;
    case MUNMAP: munmap(b->x,b->a); break;
    default: ;
  }
  
  b->x = NULL;
  b->n = 0;
  b->p = 0;
  b->a = 0;
}


