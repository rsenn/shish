#include "../../lib/byte.h"
#include "../history.h"

unsigned int history_offset;
unsigned int history_count;

/* advance
 * ----------------------------------------------------------------------- */
void
history_advance(void) {
  if(history_array[history_size - 1])
    history_free(history_size - 1);

  byte_copyr(&history_array[1], (history_size - 1) * sizeof(char*), history_array);

  history_array[0] = NULL;
  history_count++;
  history_offset = 0;
}
