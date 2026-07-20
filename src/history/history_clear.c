#include "../../lib/alloc.h"
#include "../history.h"

/* clear the in-memory session history (matches other shells' `history
 * -c`: it does not touch the on-disk file, only what's been
 * remembered/typed so far this session).
 * ----------------------------------------------------------------------- */
void
history_clear(void) {
  unsigned int i;

  for(i = 0; i < history_session_count; i++)
    alloc_free(history_session[(history_session_head + i) % history_session_cap]);

  history_session_count = 0;
  history_session_head = 0;
  history_cursor = 0;
  stralloc_zero(&history_pending);
}
