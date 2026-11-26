#ifndef RE_COMPAT_H
#define RE_COMPAT_H

/* one regex API over two engines: text/dfa (default) or the system <regex.h>
 * (USE_SYSTEM_REGEX). Only features both have are exposed; all positions are
 * byte offsets into the subject, which need not be NUL-terminated (with the
 * system engine that needs REG_STARTEND, otherwise the subject must be).
 *
 *   rx_compile   BRE or ERE (RX_ERE), case folding (RX_ICASE), error code
 *   rx_error     message for an rx_compile error code
 *   rx_groups    number of \( \) groups
 *   rx_test      match anywhere?
 *   rx_search    leftmost-longest match at or after an offset
 *   rx_prefix    longest match anchored at s[0]
 *   rx_submatch  spans of groups 1..ng (ng <= 9) inside a known match
 *
 * Named rx_ because glibc's <regex.h> owns re_search() and RE_ICASE.
 * ----------------------------------------------------------------------- */

#include <stddef.h>
#include "../lib/str.h"

#if USE_SYSTEM_REGEX
#include <regex.h>
typedef regex_t rx_t;

enum { RX_ERE = REG_EXTENDED, RX_ICASE = REG_ICASE, RX_NOTBOL = REG_NOTBOL };
#else
#include "../text/dfa.h"
typedef struct dfa rx_t;

enum { RX_ERE = DFA_ERE, RX_ICASE = DFA_ICASE, RX_NOTBOL = DFA_NOTBOL };
#endif

struct rx_span {
  size_t start; /* first byte of the match */
  size_t end;   /* one past the last byte; start == end is an empty match */
};

#define RX_NOGROUP ((size_t)-1) /* span of a group that did not take part */

/* run the system matcher on s[from..len), results as absolute offsets in pm.
   `from` > 0 means ^ must not match there, as in the dfa engine. */
#if USE_SYSTEM_REGEX
static inline int
rx_exec(rx_t* r, const char* s, size_t len, size_t from, unsigned flags, size_t n, regmatch_t* pm) {
  int ef = (int)(flags & RX_NOTBOL) | (from ? REG_NOTBOL : 0);
  size_t i;

#ifdef REG_STARTEND
  pm[0].rm_so = (regoff_t)from;
  pm[0].rm_eo = (regoff_t)len;
  if(regexec(r, s, n, pm, ef | REG_STARTEND) != 0)
    return 0;
#else
  (void)len;
  if(regexec(r, s + from, n, pm, ef) != 0)
    return 0;
  for(i = 0; i < n; i++)
    if(pm[i].rm_so >= 0) {
      pm[i].rm_so += (regoff_t)from;
      pm[i].rm_eo += (regoff_t)from;
    }
#endif
  (void)i;
  return pm[0].rm_so >= 0;
}
#endif

static inline void
rx_init(rx_t* r) {
#if USE_SYSTEM_REGEX
  (void)r;
#else
  *r = (struct dfa){0};
#endif
}

/* compile the NUL-terminated pattern; returns 0 or an error code for rx_error() */
static inline int
rx_compile(rx_t* r, const char* pat, unsigned flags) {
#if USE_SYSTEM_REGEX
  return regcomp(r, pat, (int)flags);
#else
  rx_init(r);
  return dfa_compile(r, pat, str_len(pat), flags);
#endif
}

/* message for an rx_compile() error code (valid until the next call) */
static inline const char*
rx_error(const rx_t* r, int code) {
#if USE_SYSTEM_REGEX
  static char msg[128];

  regerror(code, r, msg, sizeof(msg));
  return msg;
#else
  (void)r;
  return dfa_error(code);
#endif
}

static inline void
rx_free(rx_t* r) {
#if USE_SYSTEM_REGEX
  regfree(r);
#else
  dfa_free(r);
#endif
}

static inline unsigned
rx_groups(rx_t* r) {
#if USE_SYSTEM_REGEX
  return (unsigned)r->re_nsub;
#else
  return (unsigned)dfa_groups(r);
#endif
}

/* any match anywhere in s[0..len) */
static inline int
rx_test(rx_t* r, const char* s, size_t len) {
#if USE_SYSTEM_REGEX
  regmatch_t pm[1];

  return rx_exec(r, s, len, 0, 0, 1, pm);
#else
  return dfa_test(r, s, len) != 0;
#endif
}

/* leftmost-longest match at or after `from`; flags: RX_NOTBOL */
static inline int
rx_search(rx_t* r, const char* s, size_t len, size_t from, unsigned flags, struct rx_span* out) {
#if USE_SYSTEM_REGEX
  regmatch_t pm[1];

  if(!rx_exec(r, s, len, from, flags, 1, pm))
    return 0;
  if(out) {
    out->start = (size_t)pm[0].rm_so;
    out->end = (size_t)pm[0].rm_eo;
  }
  return 1;
#else
  struct dfa_span m;
  unsigned prev = r->flags;
  int ok;

  r->flags = (r->flags & ~DFA_NOTBOL) | (flags & RX_NOTBOL);
  ok = dfa_search(r, s, len, from, &m);
  r->flags = prev;
  if(!ok)
    return 0;
  if(out) {
    out->start = m.start;
    out->end = m.end;
  }
  return 1;
#endif
}

/* length of the longest match anchored at s[0], or -1; 0 is an empty match */
static inline long
rx_prefix(rx_t* r, const char* s, size_t len) {
#if USE_SYSTEM_REGEX
  regmatch_t pm[1];

  if(!rx_exec(r, s, len, 0, 0, 1, pm) || pm[0].rm_so != 0)
    return -1;
  return (long)pm[0].rm_eo;
#else
  return dfa_prefix(r, s, len);
#endif
}

/* fill g[0..ng) with the spans of groups 1..ng inside the match m that
   rx_search/rx_prefix found; a group that did not take part gets RX_NOGROUP */
static inline int
rx_submatch(rx_t* r, const char* s, size_t len, const struct rx_span* m, struct rx_span* g, size_t ng) {
#if USE_SYSTEM_REGEX
  regmatch_t pm[10];
  size_t i;

  if(ng > 9)
    ng = 9;
  if(!rx_exec(r, s, len, m->start, 0, ng + 1, pm))
    return 0;
  for(i = 0; i < ng; i++) {
    g[i].start = pm[i + 1].rm_so < 0 ? RX_NOGROUP : (size_t)pm[i + 1].rm_so;
    g[i].end = pm[i + 1].rm_so < 0 ? RX_NOGROUP : (size_t)pm[i + 1].rm_eo;
  }
  return 1;
#else
  return dfa_submatch(r, s, len, (const struct dfa_span*)m, (struct dfa_span*)g, ng);
#endif
}

#endif
