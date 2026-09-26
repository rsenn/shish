#include "dfa_prog.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

/* dfa_emit: append one instruction, growing the program array as
 * needed. Returns the new instruction's pc, or -1 on allocation
 * failure or DFA_MAXPROG overflow (ps->err is set either way).
 * ----------------------------------------------------------------------- */
int
dfa_emit(struct dfa_parser* ps, int op, int x, int y) {
  if(ps->err)
    return -1;

  if(ps->proglen >= DFA_MAXPROG) {
    ps->err = DFA_ESIZE;
    return -1;
  }

  if(ps->proglen == ps->progcap) {
    size_t newcap = ps->progcap ? ps->progcap * 2 : 64;
    struct dfa_inst* p = alloc_re(ps->prog, newcap * sizeof(*p));

    if(!p) {
      ps->err = DFA_ENOMEM;
      return -1;
    }

    ps->prog = p;
    ps->progcap = newcap;
  }

  ps->prog[ps->proglen].op = (unsigned char)op;
  ps->prog[ps->proglen].x = x;
  ps->prog[ps->proglen].y = y;
  return (int)ps->proglen++;
}

/* dfa_addset: interns a 32-byte bitset (dedup by content) and returns
 * its index, or -1 on allocation failure.
 * ----------------------------------------------------------------------- */
int
dfa_addset(struct dfa_parser* ps, const unsigned char set[32]) {
  size_t i;

  if(ps->err)
    return -1;

  for(i = 0; i < ps->nsets; i++)
    if(byte_diff(ps->sets[i], 32, set) == 0)
      return (int)i;

  if(ps->nsets == ps->setscap) {
    size_t newcap = ps->setscap ? ps->setscap * 2 : 16;
    unsigned char (*p)[32] = alloc_re(ps->sets, newcap * 32);

    if(!p) {
      ps->err = DFA_ENOMEM;
      return -1;
    }

    ps->sets = p;
    ps->setscap = newcap;
  }

  byte_copy(ps->sets[ps->nsets], 32, set);
  return (int)ps->nsets++;
}
