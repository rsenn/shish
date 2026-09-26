#include "../trace.h"

#ifdef DEBUG_OUTPUT
#include <signal.h>

/* events queued from signal handlers, printed later from ordinary context.
 *
 *   trace_defer()  handler side: fills the next slot, drops on overflow
 *   trace_flush()  normal side:  prints and frees every queued slot
 * ----------------------------------------------------------------------- */
#define TRACE_DEFER_SLOTS 64

static struct {
  enum trace_module mod;
  const char *event, *key;
  long val;
} trace_slots[TRACE_DEFER_SLOTS];

static volatile sig_atomic_t trace_head, trace_tail; /* head: next write, tail: next read */

/* only async-signal-safe operations: array stores and one index update */
void
trace_defer(enum trace_module mod, const char* event, const char* key, long val) {
  int h = trace_head, n = (h + 1) % TRACE_DEFER_SLOTS;

  if(n == trace_tail)
    return;

  trace_slots[h].mod = mod;
  trace_slots[h].event = event;
  trace_slots[h].key = key;
  trace_slots[h].val = val;
  trace_head = n;
}

void
trace_flush(void) {
  while(trace_tail != trace_head) {
    int t = trace_tail;
    enum trace_module mod = trace_slots[t].mod;
    const char *event = trace_slots[t].event, *key = trace_slots[t].key;
    long val = trace_slots[t].val;

    trace_tail = (t + 1) % TRACE_DEFER_SLOTS;

    TRACE(mod, event, trace_int(key, val));
  }
}
#endif
