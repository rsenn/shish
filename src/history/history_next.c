#include "../history.h"
#include "../term.h"

/* put the next entry of the history into the cmdline
 * ----------------------------------------------------------------------- */
void
history_next(void) {
  if(history_offset > 0) {
    unsigned long len = 0;
    char* p;

    history_offset--;

    if((p = history_array[history_offset]))
      len = history_cmdlen(p);

    term_setline(p, len);
  }
}
