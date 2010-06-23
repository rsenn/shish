#include <buffer.h>
#include <shell.h>
#include <unistd.h>
#include <sys/mman.h>

void buffer_close(buffer* b) {
  if (b->fd != -1) close(b->fd);
  switch (b->todo) {
  case FREE: shell_free(b->x); break;
  case MUNMAP:
#ifdef __MINGW32__
    UnmapViewOfFile(b->x);
#else
    munmap(b->x,b->a); break;
#endif
  default: ;
  }
}
