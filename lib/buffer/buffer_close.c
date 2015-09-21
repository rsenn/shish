#include "buffer.h"
#include "mmap.h"
#include <stdlib.h>
#include <unistd.h>

void buffer_close(buffer* b) {
  if (b->fd != -1) close(b->fd);
  switch (b->todo) {
  case FREE: free(b->x); break;
  case MUNMAP: mmap_unmap(b->x,b->a); break;
  default: ;
  }
}
