#include "../../lib/alloc.h"
#include "../../lib/scan.h"
#include "../history.h"
#include "../var.h"

/* (re)size the in-memory session ring to match $HISTSIZE, dropping the
 * oldest entries if it shrank. called once at startup and again before
 * every history_add, so changing $HISTSIZE interactively takes effect
 * immediately, same as other shells.
 * ----------------------------------------------------------------------- */
void
history_session_resize(void) {
  unsigned long newcap = DEFAULT_HISTSIZE;
  char** ring;
  unsigned int i, n;

  scan_ulong(var_value("HISTSIZE", NULL), &newcap);

  if(newcap == 0)
    newcap = DEFAULT_HISTSIZE;

  if(newcap == history_session_cap)
    return;

  n = history_session_count;

  if(n > newcap)
    n = (unsigned int)newcap;

  ring = alloc(newcap * sizeof(char*));

  /* free whatever doesn't make the cut (the oldest entries) */
  for(i = 0; i < history_session_count - n; i++)
    alloc_free(history_session[(history_session_head + i) % history_session_cap]);

  /* copy over the n newest entries, oldest-of-those-kept first */
  for(i = 0; i < n; i++) {
    unsigned int src =
        (history_session_head + (history_session_count - n) + i) % history_session_cap;
    ring[i] = history_session[src];
  }

  alloc_free(history_session);

  history_session = ring;
  history_session_cap = (unsigned int)newcap;
  history_session_count = n;
  history_session_head = 0;
}
