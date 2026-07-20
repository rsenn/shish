#include "../../lib/str.h"
#include "../history.h"
#include "../term.h"

/* move to an older history entry (up arrow). the very first step off
 * the live edit line stashes its current (possibly unfinished)
 * contents in history_pending so it can be restored on the way back
 * down. moving further back into the file only ever costs one more
 * step of the backward-scan cache (history_file_entry) -- never a
 * rescan of everything already visited, and never anything beyond the
 * single new entry needed.
 * ----------------------------------------------------------------------- */
void
history_prev(void) {
  unsigned int next = history_cursor + 1;

  if(history_cursor == 0) {
    char* live = term_getline();

    stralloc_free(&history_pending);

    if(live) {
      history_pending.s = live;
      history_pending.len = str_len(live);
      history_pending.a = history_pending.len + 1;
    } else
      stralloc_init(&history_pending);
  }

  if(next <= history_session_count) {
    history_cursor = next;
    history_show(history_cursor);
    return;
  }

  {
    const char* line;
    size_t len;
    unsigned int file_idx = next - history_session_count - 1;

    if(history_file_entry(file_idx, &line, &len)) {
      history_cursor = next;
      history_show(history_cursor);
    }
    /* else: already showing the oldest entry, nothing further back */
  }
}
