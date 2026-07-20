#include "../../lib/alloc.h"
#include "../../lib/buffer.h"
#include "../../lib/byte.h"
#include "../../lib/open.h"
#include "../../lib/stralloc.h"
#include "../../lib/windoze.h"
#include "../history.h"
#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

/* append one already-encoded line (plus trailing newline) to the
 * history file. opens/writes/closes on every call rather than keeping
 * an fd around: history_add only happens once per interactive command,
 * so this is not hot, and it means nothing needs to flush at exit.
 * ----------------------------------------------------------------------- */
static void
history_file_append(const stralloc* line) {
  buffer b;
  int fd = open_append(history_file_path);

  if(fd == -1)
    return;

  b.fd = fd;
  b.p = b.n = 0;
  b.a = 0;
  b.x = NULL;
  b.op = (buffer_op_proto*)(void*)&write;
  b.deinit = NULL;
  b.cookie = NULL;

  buffer_put(&b, line->s, line->len);
  buffer_puts(&b, "\n");
  buffer_flush(&b);
  close(fd);
}

/* record one completed interactive command: push it onto the session
 * ring (so it's immediately recallable) and append it to the history
 * file (so it survives into the next session). resets browsing state.
 * ----------------------------------------------------------------------- */
void
history_add(const char* s, size_t len) {
  stralloc encoded = {0, 0, 0};
  unsigned int idx;

  if(len == 0)
    return;

  history_session_resize();

  if(history_session_count == history_session_cap) {
    alloc_free(history_session[history_session_head]);
    history_session_head = (history_session_head + 1) % history_session_cap;
    history_session_count--;
  }

  idx = (history_session_head + history_session_count) % history_session_cap;
  history_session[idx] = alloc(len + 1);
  byte_copy(history_session[idx], len, s);
  history_session[idx][len] = '\0';
  history_session_count++;

  history_encode(s, len, &encoded);
  history_file_append(&encoded);
  stralloc_free(&encoded);

  history_cursor = 0;
  stralloc_zero(&history_pending);
}
