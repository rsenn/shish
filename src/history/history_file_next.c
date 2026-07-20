#include "../../lib/byte.h"
#include "../history.h"

/* forward line iterator: on entry *pos is the offset to read the next
 * entry from (0 for the very first call); returns the entry there and
 * advances *pos past it (and its terminating newline). returns 0 at
 * end of file. used to print the whole (on-disk) history oldest-first
 * without needing the backward-scan cache to have been populated.
 * ----------------------------------------------------------------------- */
int
history_file_next(size_t* pos, const char** line, size_t* len) {
  size_t nl;

  if(*pos >= history_file_len)
    return 0;

  nl = byte_chr(history_file_map + *pos, history_file_len - *pos, '\n');

  *line = history_file_map + *pos;

  if(nl == history_file_len - *pos) {
    /* no trailing newline on the last line */
    *len = nl;
    *pos = history_file_len;
  } else {
    *len = nl;
    *pos += nl + 1;
  }

  return 1;
}
