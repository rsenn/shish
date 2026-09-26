#include "dfa_prog.h"
#include "../../lib/alloc.h"
#include "../../lib/arena.h"

struct dfa_thread {
  int pc;
  long* save; /* 2*ngroup longs, immutable once created, arena-owned */
};

struct dfa_tlist {
  struct dfa_thread* t;
  size_t n;
};

/* addthread: epsilon-closure from pc, added to *l with save as the
 * capture state so far. mark[]/gen dedup pc's already reached this
 * step, which both avoids double-counting a thread and bounds
 * recursion depth by the program size (not the input length) --
 * a cyclic SPLIT (`x*`) can only be entered once per step.
 * ----------------------------------------------------------------------- */
static void
addthread(arena* a,
          struct dfa_tlist* l,
          int* mark,
          int gen,
          const struct dfa_inst* prog,
          size_t ngroup,
          int pc,
          long* save,
          size_t pos,
          int notbol,
          size_t n) {
  if(mark[pc] == gen)
    return;

  mark[pc] = gen;

  switch(prog[pc].op) {
    case DFA_JMP:
      addthread(a, l, mark, gen, prog, ngroup, prog[pc].x, save, pos, notbol, n);
      return;

    case DFA_SPLIT:
      addthread(a, l, mark, gen, prog, ngroup, prog[pc].x, save, pos, notbol, n);
      addthread(a, l, mark, gen, prog, ngroup, prog[pc].y, save, pos, notbol, n);
      return;

    case DFA_SAVE:
      if(ngroup) {
        long* ns = arena_dup(a, save, 2 * ngroup * sizeof(long));

        ns[prog[pc].x] = (long)pos;
        addthread(a, l, mark, gen, prog, ngroup, pc + 1, ns, pos, notbol, n);
      } else {
        addthread(a, l, mark, gen, prog, ngroup, pc + 1, save, pos, notbol, n);
      }
      return;

    case DFA_BOL:
      if(pos == 0 && !notbol)
        addthread(a, l, mark, gen, prog, ngroup, pc + 1, save, pos, notbol, n);
      return;

    case DFA_EOL:
      if(pos == n)
        addthread(a, l, mark, gen, prog, ngroup, pc + 1, save, pos, notbol, n);
      return;

    default: /* CHAR/ANY/SET/BACKREF/MATCH: consumes input or ends the match */
      l->t[l->n].pc = pc;
      l->t[l->n].save = save;
      l->n++;
      return;
  }
}

/* dfa_run: Pike's thread-based NFA simulation. Tries each start
 * position from `start` on (or only `start`, if anchored); at the
 * first one with any match, runs every live thread to exhaustion and
 * keeps the longest, which is POSIX leftmost-longest. A pattern with
 * a back-reference never reaches here (see dfa_bt_run).
 * ----------------------------------------------------------------------- */
int
dfa_run(const struct dfa* d,
        const char* s,
        size_t n,
        size_t start,
        int anchored,
        int notbol,
        struct dfa_span* m,
        struct dfa_span* g,
        size_t ng) {
  const struct dfa_inst* prog = d->prog;
  const unsigned char (*sets)[32] = d->sets;
  size_t proglen = d->proglen;
  arena a;
  int* mark;
  struct dfa_tlist clist, nlist;
  size_t from, i;
  int gen = 0;
  int ok = 0;

  if(proglen == 0)
    return 0;

  arena_init(&a, &arena_heap, 4096);
  mark = alloc(proglen * sizeof(int));
  clist.t = alloc(proglen * sizeof(struct dfa_thread));
  nlist.t = alloc(proglen * sizeof(struct dfa_thread));

  if(!mark || !clist.t || !nlist.t)
    goto out;

  for(i = 0; i < proglen; i++)
    mark[i] = -1;

  for(from = start;; from++) {
    long* save = NULL;
    long best_end = -1;
    long* best_save = NULL;
    size_t pos;

    if(d->ngroup) {
      save = arena_alloc(&a, 2 * d->ngroup * sizeof(long), sizeof(long));
      for(i = 0; i < 2 * d->ngroup; i++)
        save[i] = -1;
    }

    clist.n = 0;
    gen++;
    addthread(&a, &clist, mark, gen, prog, d->ngroup, 0, save, from, notbol, n);

    for(pos = from;; pos++) {
      int c = (pos < n) ? (unsigned char)s[pos] : -1;

      if(clist.n == 0)
        break;

      nlist.n = 0;
      gen++;

      for(i = 0; i < clist.n; i++) {
        int pc = clist.t[i].pc;
        long* sv = clist.t[i].save;

        switch(prog[pc].op) {
          case DFA_MATCH:
            if((long)pos > best_end) {
              best_end = (long)pos;
              best_save = sv;
            }
            break;

          case DFA_CHAR:
            if(c == prog[pc].x)
              addthread(&a, &nlist, mark, gen, prog, d->ngroup, pc + 1, sv, pos + 1, notbol, n);
            break;

          case DFA_ANY:
            if(c >= 0)
              addthread(&a, &nlist, mark, gen, prog, d->ngroup, pc + 1, sv, pos + 1, notbol, n);
            break;

          case DFA_SET:
            if(c >= 0 && (sets[prog[pc].x][c >> 3] & (unsigned char)(1u << (c & 7))))
              addthread(&a, &nlist, mark, gen, prog, d->ngroup, pc + 1, sv, pos + 1, notbol, n);
            break;

          default:
            /* DFA_BACKREF: variable-length, not representable as a
               single per-character NFA step -- d->has_backref routes
               these patterns to dfa_bt_run instead, so unreachable. */
            break;
        }
      }

      {
        struct dfa_tlist tmp = clist;

        clist = nlist;
        nlist = tmp;
      }

      if(pos >= n)
        break;
    }

    if(best_end >= 0) {
      m->start = from;
      m->end = (size_t)best_end;

      for(i = 0; i < ng; i++) {
        if(i < d->ngroup && best_save && best_save[2 * i] >= 0 && best_save[2 * i + 1] >= 0) {
          g[i].start = (size_t)best_save[2 * i];
          g[i].end = (size_t)best_save[2 * i + 1];
        } else {
          g[i].start = (size_t)-1;
          g[i].end = (size_t)-1;
        }
      }

      ok = 1;
      goto out;
    }

    if(anchored || from >= n)
      break;
  }

out:
  alloc_free(mark);
  alloc_free(clist.t);
  alloc_free(nlist.t);
  arena_free(&a);
  return ok;
}
