#include "../debug.h"

#ifdef DEBUG
#include "../fd.h"

void
debug_space(int count, int newline) {
  if(newline) {
    buffer_put(fd_err->w, "\n", 1);

    if(count < 0)
      buffer_putnspace(fd_err->w, (-count) * DEBUG_SPACE);
    else
      buffer_putnspace(fd_err->w, count * DEBUG_SPACE + 1);
  } else {
    buffer_putspace(fd_err->w);
  }
}
#endif /* DEBUG */
