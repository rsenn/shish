#include "history.h"
#include "term.h"

/* put the previous entry of the history into the cmdline
 * ----------------------------------------------------------------------- */
void history_prev(void) {
  if(history_offset < history_count) {
    char *p;
    unsigned long len = 0;

    if(history_offset == 0)
      history_set(term_getline());

    history_offset++;

    if((p = history_array[history_offset]))
      len = history_cmdlen(p);

    term_setline(p, len);
  }
}

