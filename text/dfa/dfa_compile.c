#include "dfa_prog.h"
#include "../../lib/alloc.h"

/* dfa_compile: parse pat into *d's bytecode. Frees a prior compile in
 * *d first, so recompiling an already-used struct dfa is safe.
 * ----------------------------------------------------------------------- */
int
dfa_compile(struct dfa* d, const char* pat, size_t patlen, unsigned flags) {
  struct dfa_parser ps;

  dfa_free(d);

  ps.p = pat;
  ps.end = pat + patlen;
  ps.start = pat;
  ps.flags = flags;
  ps.err = DFA_OK;
  ps.ngroup = 0;
  ps.nest = 0;
  ps.prog = NULL;
  ps.proglen = 0;
  ps.progcap = 0;
  ps.sets = NULL;
  ps.nsets = 0;
  ps.setscap = 0;
  ps.has_backref = 0;

  dfa_parse_pattern(&ps);

  if(ps.err) {
    alloc_free(ps.prog);
    alloc_free(ps.sets);
    return ps.err;
  }

  d->prog = ps.prog;
  d->proglen = ps.proglen;
  d->sets = ps.sets;
  d->nsets = ps.nsets;
  d->ngroup = ps.ngroup;
  d->has_backref = ps.has_backref;
  d->flags = flags;
  return DFA_OK;
}
