#include "../debug.h"
#include "../fd.h"

void
debug_space(int count, int newline) {
  if(!newline && debug_buffer.n == debug_buffer.p)
    return;

  if(newline) {
    debug_b("\n", 1);

    if(count < 0)
      ; /*buffer_putspace(debug_output);*/
    else
      buffer_putnspace(debug_output, count * DEBUG_SPACE + 1);
  } else {
    buffer_putspace(debug_output);
  }

  debug_fl();
}
