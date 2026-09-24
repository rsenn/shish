#include "dfa_prog.h"

/* dfa_submatch: re-run anchored at m->start. dfa_run/dfa_bt_run
 * always take the longest match from a given start, and m->end is
 * exactly that longest match (dfa_test/dfa_prefix/dfa_search never
 * hand back a shorter one), so re-anchoring there reproduces the
 * same match and its capture groups without threading g[] through
 * every earlier call.
 * ----------------------------------------------------------------------- */
int
dfa_submatch(
    struct dfa* d, const char* s, size_t n, const struct dfa_span* m, struct dfa_span* g, size_t ng) {
  struct dfa_span got;
  int notbol = (d->flags & DFA_NOTBOL) != 0;
  int ok = d->has_backref ? dfa_bt_run(d, s, n, m->start, 1, notbol, &got, g, ng)
                           : dfa_run(d, s, n, m->start, 1, notbol, &got, g, ng);

  return ok && got.end == m->end;
}
