#include "buffer.h"
#include <stdlib.h>
#include <unistd.h>
<<<<<<< HEAD
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
=======
#include "mmap.h"
>>>>>>> 1b3740a7bdef6c31ba1529670c639e3ff7923f56

void buffer_close(buffer* b) {
  if (b->fd != -1) close(b->fd);
  switch (b->todo) {
  case FREE: free(b->x); break;
  case MUNMAP: mmap_unmap(b->x,b->a); break;
  default: ;
  }
}
