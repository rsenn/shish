#include "../history.h"
#include "../term.h"

/* move to a newer history entry (down arrow), eventually landing back
 * on the live edit line as it was before browsing started.
 * ----------------------------------------------------------------------- */
void
history_next(void) {
  if(history_cursor == 0)
    return;

  history_cursor--;

  if(history_cursor == 0)
    term_setline(history_pending.s, history_pending.len);
  else
    history_show(history_cursor);
}
