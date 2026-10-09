#include "../../lib/byte.h"
#include "../fd.h"
#include "../trace.h"
#include "../eval.h"
#include "../expand.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../source.h"
#include "../vartab.h"

struct eval* eval = NULL;

void
eval_push(struct eval* e, int flags) {
  byte_zero(e, sizeof(struct eval));
  e->flags = flags | (sh->opts.xtrace ? E_PRINT : 0);
  e->parent = eval;

  /* remember stack locations in current nesting level */
  e->fdstack = fdstack;
  e->varstack = varstack;
  e->source = source;
  e->apos = arena_tell(&expand_arena);
  e->pool = wordlist_pool_mark();

  eval = e;

  TRACE(TRACE_EVAL, "push", trace_flags("flags", e->flags, trace_eval_flags, 11));
}
