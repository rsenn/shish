#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

void
debug_space(int count, int newline) {
  if(!newline && debug_buffer.n == debug_buffer.p)
    return;
  if(newline) {
    debug_b("\n", 1);
    if(count < 0)
      ; //   buffer_putspace(&debug_buffer);
    else
      buffer_putnspace(&debug_buffer, count * DEBUG_SPACE + 1);
  } else {
    buffer_putspace(&debug_buffer);
  }
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
