#include "dfa_prog.h"
#include "../../lib/byteset.h"

/* dfa_bracket_compile: ps->p must point at the opening '['. Fills
 * set, advances ps->p past the closing ']'. 1 on success, 0 on error
 * (ps->err set to a DFA_E* code).
 * ----------------------------------------------------------------------- */
int
dfa_bracket_compile(struct dfa_parser* ps, unsigned char set[32]) {
  const char* p = ps->p + 1;
  const char* end = ps->end;
  int neg = 0, first = 1;

  byteset_zero(set);

  if(p < end && *p == '^') {
    neg = 1;
    p++;
  }

  while(p < end && !(*p == ']' && !first)) {
    if(p[0] == '[' && p + 1 < end && (p[1] == ':' || p[1] == '.' || p[1] == '=')) {
      char delim = p[1];
      const char* q = p + 2;

      while(q + 1 < end && !(q[0] == delim && q[1] == ']'))
        q++;

      if(!(q + 1 < end && q[0] == delim && q[1] == ']')) {
        ps->err = DFA_EBRACKET;
        return 0;
      }

      if(delim == ':') {
        if(!byteset_class(set, p + 2, (size_t)(q - (p + 2)))) {
          ps->err = DFA_ECLASS;
          return 0;
        }
      } else {
        /* [.x.] / [=x=]: only single-character collating symbols */
        if(q - (p + 2) != 1) {
          ps->err = DFA_EBRACKET;
          return 0;
        }

        byteset_add(set, (unsigned char)p[2]);
      }

      p = q + 2;
      first = 0;
      continue;
    }

    if(p + 2 < end && p[1] == '-' && p[2] != ']') {
      if(byteset_range(set, (unsigned char)p[0], (unsigned char)p[2]) < 0) {
        ps->err = DFA_ERANGE;
        return 0;
      }

      p += 3;
      first = 0;
      continue;
    }

    byteset_add(set, (unsigned char)*p);
    p++;
    first = 0;
  }

  if(!(p < end && *p == ']')) {
    ps->err = DFA_EBRACKET;
    return 0;
  }

  p++;

  if(neg)
    byteset_invert(set);

  if(ps->flags & DFA_ICASE)
    byteset_icase(set);

  ps->p = p;
  return 1;
}
