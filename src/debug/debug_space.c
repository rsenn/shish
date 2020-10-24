#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

void
debug_space(int count, int newline) {
  if(!newline && buffer_2->n == buffer_2->p)
    return;
  if(newline) {
    debug_b("\n", 1);
    if(count < 0)
      ; //   buffer_putspace(buffer_2);
    else
      buffer_putnspace(buffer_2, count * DEBUG_SPACE + 1);
  } else {
    buffer_putspace(buffer_2);
  }
  buffer_flush(buffer_2);
}
#endif /* DEBUG_OUTPUT */
