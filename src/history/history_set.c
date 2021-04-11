#include "../history.h"
#include "../../lib/alloc.h"

/* add a command to the history
 * ----------------------------------------------------------------------- */
void
history_set(char* s) {
  history_resize();

  if(history_array[0])
    alloc_free(history_array[0]);

  if(s == NULL || *s)
    history_array[0] = s;
  else if(s < history_buffer.x || s >= history_buffer.x + history_buffer.n)
    alloc_free(s);
}
