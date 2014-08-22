<<<<<<< HEAD
#include <shell.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
=======
#include "shell.h"
>>>>>>> 1b3740a7bdef6c31ba1529670c639e3ff7923f56
#include "buffer.h"
#include "mmap.h"

void buffer_free(buffer *b)
{
  switch (b->todo)
  {
    case FREE: shell_free(b->x); break;
    case MUNMAP: mmap_unmap(b->x,b->a); break;
    default: ;
  }
  
  b->x = NULL;
  b->n = 0;
  b->p = 0;
  b->a = 0;
}


