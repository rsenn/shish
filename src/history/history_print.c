#include "../../lib/fmt.h"
#include "../../lib/str.h"
#include "../fdtable.h"
#include "../history.h"

static void
history_print_line(unsigned long n, const char* s, size_t len) {
  char numstr[FMT_ULONG];
  size_t nn = fmt_ulong(numstr, n);

  buffer_putnspace(fd_out->w, nn < 5 ? 5 - nn : 0);
  buffer_put(fd_out->w, numstr, nn);
  buffer_putnspace(fd_out->w, 2);
  buffer_put(fd_out->w, s, len);
  buffer_putnlflush(fd_out->w);
}

/* list the whole history, oldest first and numbered, like other
 * shells' `history` builtin: the on-disk entries (decoded back from
 * their one-line-per-entry escaped form) followed by whatever's been
 * entered so far this session. this is the one operation that
 * legitimately walks the entire file -- it's what the user asked for.
 * ----------------------------------------------------------------------- */
void
history_print(void) {
  unsigned long n = 1;
  size_t pos = 0;
  const char* line;
  size_t len;
  unsigned int i;

  while(history_file_next(&pos, &line, &len)) {
    stralloc decoded = {0, 0, 0};

    history_decode(line, len, &decoded);
    history_print_line(n++, decoded.s, decoded.len);
    stralloc_free(&decoded);
  }

  for(i = 0; i < history_session_count; i++) {
    const char* s = history_session[(history_session_head + i) % history_session_cap];

    history_print_line(n++, s, str_len(s));
  }
}
