#include "buffer.h"
#include <stdlib.h>
<<<<<<< HEAD
#ifndef WIN32
=======
#ifdef _WIN32
#include <io.h>
#else
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
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
