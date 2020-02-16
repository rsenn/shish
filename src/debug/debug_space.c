#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

void
debug_space(int count, int newline) {

  if(!newline && fd_err->w->n == fd_err->w->p)
    return;

  if(newline) {
    buffer_put(fd_err->w, "\n", 1);
    if(count < 0) {
      buffer_putspace(fd_err->w);
    } else {
      buffer_putnspace(fd_err->w, count * DEBUG_SPACE + 1);
    }
  } else {
    buffer_putspace(fd_err->w);
  }
  buffer_flush(fd_err->w);
}
#endif /* DEBUG_OUTPUT */
