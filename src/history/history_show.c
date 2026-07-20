#include "../../lib/str.h"
#include "../history.h"
#include "../term.h"

/* puts the entry <cursor> positions back onto the edit line (cursor
 * counts from 1 = newest; the first history_session_count values come
 * from the in-memory session ring, older ones are decoded on demand
 * from the on-disk file cache).
 * ----------------------------------------------------------------------- */
void
history_show(unsigned int cursor) {
  if(cursor >= 1 && cursor <= history_session_count) {
    unsigned int idx = (history_session_head + history_session_count - cursor) % history_session_cap;
    const char* s = history_session[idx];

    term_setline(s, str_len(s));
  } else {
    unsigned int file_idx = cursor - history_session_count - 1;
    const char* line;
    size_t len;

    if(history_file_entry(file_idx, &line, &len)) {
      stralloc decoded = {0, 0, 0};

      history_decode(line, len, &decoded);
      term_setline(decoded.s, decoded.len);
      stralloc_free(&decoded);
    }
  }
}
