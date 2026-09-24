/* internal to the files in this directory: the compiled program and
 * parser state. Not installed, not included from outside text/dfa/.
 * ----------------------------------------------------------------------- */
#ifndef TEXT_DFA_PROG_H
#define TEXT_DFA_PROG_H

#include "../dfa.h"

#define DFA_MAXGROUPS 32   /* recorded; only \1-\9 are addressable */
#define DFA_MAXNEST 256    /* ( nesting depth */
#define DFA_MAXDUP 255     /* largest m or n in \{m,n\} (RE_DUP_MAX) */
#define DFA_MAXPROG 32767  /* instructions after {m,n} expansion */
#define DFA_MAXSTEPS 10000000 /* backtracker step budget */

enum dfa_op {
  DFA_CHAR = 1, /* x = byte to match */
  DFA_ANY,
  DFA_SET,      /* x = index into d->sets (32-byte bitset each) */
  DFA_SPLIT,    /* try pc+1 (x) first, then y -- x/y hold absolute targets */
  DFA_JMP,      /* x = target */
  DFA_SAVE,     /* x = slot (2*group or 2*group+1) */
  DFA_BOL,
  DFA_EOL,
  DFA_BACKREF,  /* x = group number, 1-based */
  DFA_MATCH
};

struct dfa_inst {
  unsigned char op;
  int x, y;
};

/* struct dfa itself is defined in ../dfa.h (with void* in place of the
   two typed pointers below, so callers can size/zero it without
   seeing these internal types); every access here goes through
   d->prog cast to struct dfa_inst* and d->sets cast to its real type. */

/* parser: builds *d->prog in place as it recognizes the grammar */
struct dfa_parser {
  const char* p;
  const char* end;
  const char* start; /* p at the very beginning (for ^/$ position rules) */
  unsigned flags;
  int err;
  size_t ngroup;
  const char* gsrc[DFA_MAXGROUPS]; /* group index by the source pos of its
                                       '(', so re-parsing the same span for
                                       {m,n} duplication reuses the number */
  int nest;
  struct dfa_inst* prog;
  size_t proglen, progcap;
  unsigned char (*sets)[32];
  size_t nsets, setscap;
  int has_backref;
};

int dfa_emit(struct dfa_parser* ps, int op, int x, int y);
int dfa_addset(struct dfa_parser* ps, const unsigned char set[32]);
int dfa_bracket_compile(struct dfa_parser* ps, unsigned char set[32]);
void dfa_parse_pattern(struct dfa_parser* ps);

int dfa_run(const struct dfa* d, const char* s, size_t n, size_t start, int anchored, int notbol,
            struct dfa_span* m, struct dfa_span* g, size_t ng);
int dfa_bt_run(const struct dfa* d, const char* s, size_t n, size_t start, int anchored, int notbol,
               struct dfa_span* m, struct dfa_span* g, size_t ng);

#endif
