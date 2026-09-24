#include "dfa_prog.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

/* dfa_bt_run: explicit-stack backtracker for patterns with a
 * back-reference (not regular, so dfa_run's NFA simulation can't
 * handle them). Depth is bounded by these heap stacks, not the C
 * call stack, so it can't overflow on long input the way a plain
 * recursive matcher would; DFA_MAXSTEPS bounds total work instead,
 * since backtracking with a back-reference can be exponential.
 *
 * Semantics here are leftmost-first (first successful path wins),
 * not full POSIX leftmost-longest -- true longest-match with
 * back-references is exponential to decide in general. SPLIT always
 * tries its greedy (body) branch first, so `*`/`+`/`{m,n}` still
 * prefer the longest repetition among the paths actually explored.
 * ----------------------------------------------------------------------- */

struct dfa_choice {
  int pc;
  size_t pos;
  size_t undo_top;
};

struct dfa_undo {
  int slot;
  long oldval;
};

static int
bt_match(const struct dfa* d, const char* s, size_t n, size_t from, int notbol, size_t* end,
         long* save) {
  const struct dfa_inst* prog = d->prog;
  const unsigned char(*sets)[32] = d->sets;
  struct dfa_choice* cs = NULL;
  size_t cs_n = 0, cs_cap = 0;
  struct dfa_undo* us = NULL;
  size_t us_n = 0, us_cap = 0;
  int pc = 0;
  size_t pos = from;
  unsigned long steps = 0;
  int result = 0;

  for(;;) {
    int backtrack = 0;

    if(++steps > DFA_MAXSTEPS) {
      result = 0;
      break;
    }

    switch(prog[pc].op) {
    case DFA_CHAR:
      if(pos < n && (unsigned char)s[pos] == prog[pc].x) {
        pc++;
        pos++;
      } else {
        backtrack = 1;
      }
      break;

    case DFA_ANY:
      if(pos < n) {
        pc++;
        pos++;
      } else {
        backtrack = 1;
      }
      break;

    case DFA_SET:
      if(pos < n && (sets[prog[pc].x][(unsigned char)s[pos] >> 3] &
                     (unsigned char)(1u << ((unsigned char)s[pos] & 7)))) {
        pc++;
        pos++;
      } else {
        backtrack = 1;
      }
      break;

    case DFA_JMP:
      pc = prog[pc].x;
      break;

    case DFA_SPLIT:
      if(cs_n == cs_cap) {
        size_t newcap = cs_cap ? cs_cap * 2 : 64;
        struct dfa_choice* p = alloc_re(cs, newcap * sizeof(*p));

        if(!p) {
          result = 0;
          goto done;
        }
        cs = p;
        cs_cap = newcap;
      }
      cs[cs_n].pc = prog[pc].y;
      cs[cs_n].pos = pos;
      cs[cs_n].undo_top = us_n;
      cs_n++;
      pc = prog[pc].x;
      break;

    case DFA_SAVE:
      if(us_n == us_cap) {
        size_t newcap = us_cap ? us_cap * 2 : 64;
        struct dfa_undo* p = alloc_re(us, newcap * sizeof(*p));

        if(!p) {
          result = 0;
          goto done;
        }
        us = p;
        us_cap = newcap;
      }
      us[us_n].slot = prog[pc].x;
      us[us_n].oldval = save[prog[pc].x];
      us_n++;
      save[prog[pc].x] = (long)pos;
      pc++;
      break;

    case DFA_BOL:
      if(pos == 0 && !notbol)
        pc++;
      else
        backtrack = 1;
      break;

    case DFA_EOL:
      if(pos == n)
        pc++;
      else
        backtrack = 1;
      break;

    case DFA_BACKREF: {
      int g = prog[pc].x - 1;
      long gs = save[2 * g], ge = save[2 * g + 1];

      if(gs < 0 || ge < 0) {
        backtrack = 1;
      } else {
        size_t len = (size_t)(ge - gs);

        if(pos + len <= n && byte_diff(s + pos, len, s + gs) == 0) {
          pc++;
          pos += len;
        } else {
          backtrack = 1;
        }
      }
      break;
    }

    case DFA_MATCH:
      *end = pos;
      result = 1;
      goto done;

    default:
      backtrack = 1;
      break;
    }

    if(backtrack) {
      if(cs_n == 0) {
        result = 0;
        break;
      }

      cs_n--;
      pc = cs[cs_n].pc;
      pos = cs[cs_n].pos;

      while(us_n > cs[cs_n].undo_top) {
        us_n--;
        save[us[us_n].slot] = us[us_n].oldval;
      }
    }
  }

done:
  alloc_free(cs);
  alloc_free(us);
  return result;
}

int
dfa_bt_run(const struct dfa* d, const char* s, size_t n, size_t start, int anchored, int notbol,
           struct dfa_span* m, struct dfa_span* g, size_t ng) {
  size_t from, i;
  long* save = NULL;

  if(d->ngroup)
    save = alloc(2 * d->ngroup * sizeof(long));

  if(d->ngroup && !save)
    return 0;

  for(from = start;; from++) {
    size_t end;

    if(d->ngroup)
      for(i = 0; i < 2 * d->ngroup; i++)
        save[i] = -1;

    if(bt_match(d, s, n, from, notbol, &end, save)) {
      m->start = from;
      m->end = end;

      for(i = 0; i < ng; i++) {
        if(i < d->ngroup && save[2 * i] >= 0 && save[2 * i + 1] >= 0) {
          g[i].start = (size_t)save[2 * i];
          g[i].end = (size_t)save[2 * i + 1];
        } else {
          g[i].start = (size_t)-1;
          g[i].end = (size_t)-1;
        }
      }

      alloc_free(save);
      return 1;
    }

    if(anchored || from >= n)
      break;
  }

  alloc_free(save);
  return 0;
}
