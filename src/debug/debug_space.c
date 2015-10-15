#include "debug.h"

#ifdef DEBUG
#include "fd.h"

void debug_space(int count) {
  buffer_put(fd_err->w, "\n", 1);

  if(count < 0)
    buffer_putnspace(fd_err->w, (-count) * DEBUG_SPACE);
  else
    buffer_putnspace(fd_err->w, count * DEBUG_SPACE + 1);
}
#endif /* DEBUG */
