#include "dfa_prog.h"
#include "../../lib/byte.h"
#include <ctype.h>

static void
setbit(unsigned char set[32], unsigned char c) {
  set[c >> 3] |= (unsigned char)(1u << (c & 7));
}

static int
class_mark(unsigned char set[32], const char* name, size_t len) {
#define CLASS(s, fn) \
  if(len == sizeof(s) - 1 && byte_diff(name, len, s) == 0) { \
    int c; \
    for(c = 0; c < 256; c++) \
      if(fn(c)) \
        setbit(set, (unsigned char)c); \
    return 1; \
  }
  CLASS("upper", isupper)
  CLASS("lower", islower)
  CLASS("alpha", isalpha)
  CLASS("digit", isdigit)
  CLASS("alnum", isalnum)
  CLASS("punct", ispunct)
  CLASS("graph", isgraph)
  CLASS("print", isprint)
  CLASS("cntrl", iscntrl)
  CLASS("blank", isblank)
  CLASS("space", isspace)
  CLASS("xdigit", isxdigit)
#undef CLASS
  return 0;
}

/* dfa_bracket_compile: ps->p must point at the opening '['. Fills
 * set, advances ps->p past the closing ']'. 1 on success, 0 on error
 * (ps->err set to a DFA_E* code).
 * ----------------------------------------------------------------------- */
int
dfa_bracket_compile(struct dfa_parser* ps, unsigned char set[32]) {
  const char* p = ps->p + 1;
  const char* end = ps->end;
  int neg = 0, first = 1;

  byte_zero(set, 32);

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
        if(!class_mark(set, p + 2, (size_t)(q - (p + 2)))) {
          ps->err = DFA_ECLASS;
          return 0;
        }
      } else {
        /* [.x.] / [=x=]: only single-character collating symbols */
        if(q - (p + 2) != 1) {
          ps->err = DFA_EBRACKET;
          return 0;
        }

        setbit(set, (unsigned char)p[2]);
      }

      p = q + 2;
      first = 0;
      continue;
    }

    if(p + 2 < end && p[1] == '-' && p[2] != ']') {
      unsigned char lo = (unsigned char)p[0], hi = (unsigned char)p[2];
      int c;

      if(lo > hi) {
        ps->err = DFA_ERANGE;
        return 0;
      }

      for(c = lo; c <= hi; c++)
        setbit(set, (unsigned char)c);

      p += 3;
      first = 0;
      continue;
    }

    setbit(set, (unsigned char)*p);
    p++;
    first = 0;
  }

  if(!(p < end && *p == ']')) {
    ps->err = DFA_EBRACKET;
    return 0;
  }

  p++;

  if(neg) {
    int i;

    for(i = 0; i < 32; i++)
      set[i] = (unsigned char)~set[i];
  }

  if(ps->flags & DFA_ICASE) {
    int c;

    for(c = 'a'; c <= 'z'; c++)
      if(set[c >> 3] & (1u << (c & 7)))
        setbit(set, (unsigned char)(c - 'a' + 'A'));

    for(c = 'A'; c <= 'Z'; c++)
      if(set[c >> 3] & (1u << (c & 7)))
        setbit(set, (unsigned char)(c - 'A' + 'a'));
  }

  ps->p = p;
  return 1;
}
