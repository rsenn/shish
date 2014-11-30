#include "buffer.h"
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "mmap.h"

void buffer_close(buffer* b) {
  if (b->fd != -1) close(b->fd);
  switch (b->todo) {
  case FREE: free(b->x); break;
  case MUNMAP: mmap_unmap(b->x,b->a); break;
  default: ;
  }
}
