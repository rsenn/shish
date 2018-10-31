#include "history.h"

/* clear the whole history
 * ----------------------------------------------------------------------- */
void history_clear(void) {
  unsigned int i;

  for(i = 0; i < history_size; i++)
    if(history_array[i])
      history_free(i);
}

