#include "sed_internal.h"
#include "../../lib/scan.h"
#include "../../lib/stralloc.h"

/* sed_addr_parse: *pp must already look like the start of an address
 * (digit, '$', '/', or '\' -- the caller checks that before calling,
 * so it can tell "no address here" from a real parse error). Inside
 * a /re/ or \cREc address, only '\' followed by the delimiter is
 * special (it inserts a literal delimiter into the pattern); every
 * other backslash sequence passes through to the regex compiler
 * untouched. A delimiter inside a bracket expression still ends the
 * address (unlike a real RE parser); pick a different delimiter to
 * match a literal one.
 * ----------------------------------------------------------------------- */
int
sed_addr_parse(const char** pp, const char* end, struct sed_addr* a, unsigned flags) {
  const char* p = *pp;
  char delim;
  const char* start;
  stralloc pat;
  int rc = SED_OK;

  if(*p >= '0' && *p <= '9') {
    unsigned long v;
    size_t used = scan_ulong(p, &v);

    if(!used)
      return SED_EADDR;

    a->type = SA_LINE;
    a->line = v;
    p += used;
    *pp = p;
    return SED_OK;
  }

  if(*p == '$') {
    a->type = SA_LAST;
    p++;
    *pp = p;
    return SED_OK;
  }

  if(*p == '/') {
    delim = '/';
    p++;
  } else if(*p == '\\' && p + 1 < end) {
    delim = p[1];
    p += 2;
  } else {
    return SED_EADDR;
  }

  stralloc_init(&pat);
  start = p;

  while(p < end && *p != delim) {
    /* '\n' (backslash + the letter n): see sed_subst.c's pattern
       parser for why this is a newline escape here, undefined by
       POSIX BRE/ERE otherwise. */
    if(*p == '\\' && p + 1 < end && (p[1] == delim || p[1] == 'n')) {
      char lit = (p[1] == delim) ? delim : '\n';

      if(!stralloc_catb(&pat, start, (size_t)(p - start)) ||
         !stralloc_catc(&pat, (unsigned char)lit)) {
        rc = SED_ENOMEM;
        goto done;
      }

      p += 2;
      start = p;
    } else if(*p == '\\' && p + 1 < end) {
      p += 2;
    } else {
      p++;
    }
  }

  if(p >= end) {
    rc = SED_EUNTERM;
    goto done;
  }

  if(!stralloc_catb(&pat, start, (size_t)(p - start))) {
    rc = SED_ENOMEM;
    goto done;
  }

  p++;

  if(pat.len == 0) {
    a->type = SA_REGEX;
    a->re_set = 0;
  } else {
    unsigned dfaflags = (flags & SED_ERE) ? DFA_ERE : 0;
    int cc = dfa_compile(&a->re, pat.s, pat.len, dfaflags);

    if(cc != DFA_OK) {
      rc = SED_EREGEX;
      goto done;
    }

    a->type = SA_REGEX;
    a->re_set = 1;
  }

  *pp = p;

done:
  stralloc_free(&pat);
  return rc;
}

void
sed_addr_free(struct sed_addr* a) {
  if(a->type == SA_REGEX && a->re_set)
    dfa_free(&a->re);
}

int
sed_addr_match(struct sed_addr* a, struct sed_state* st) {
  switch(a->type) {
    case SA_LINE: return st->lineno == a->line;
    case SA_LAST: return st->cur_is_last;

    case SA_REGEX: {
      struct dfa* re = a->re_set ? &a->re : st->last_re;
      int m;

      if(!re)
        return 0; /* empty // with no prior regex applied yet: never matches */

      m = dfa_test(re, st->pattern.s, st->pattern.len);
      st->last_re = re;
      return m;
    }

    default: return 0;
  }
}

/* sed_range_match: naddr==0 always matches; naddr==1 tests a1;
 * naddr==2 tracks the open/closed range across calls in c->in_range
 * (see struct sed_cmd). addr2==regex is only tested starting the
 * line *after* a1 matched (POSIX): entering the range never also
 * tests a2 on the same line.
 * ----------------------------------------------------------------------- */
int
sed_range_match(struct sed_cmd* c, struct sed_state* st) {
  int result;

  if(c->naddr == 0) {
    result = 1;
  } else if(c->naddr == 1) {
    result = sed_addr_match(&c->a1, st);
  } else if(!c->in_range) {
    result = sed_addr_match(&c->a1, st);

    if(result) {
      c->in_range = 1;

      if(c->a2.type == SA_LINE) {
        if(c->a2.line <= st->lineno)
          c->in_range = 0;
      } else if(c->a2.type == SA_LAST) {
        if(st->cur_is_last)
          c->in_range = 0;
      }
    }
  } else {
    int close = 0;

    result = 1;

    if(c->a2.type == SA_LINE)
      close = st->lineno >= c->a2.line;
    else if(c->a2.type == SA_LAST)
      close = st->cur_is_last;
    else if(c->a2.type == SA_REGEX)
      close = sed_addr_match(&c->a2, st);

    if(close)
      c->in_range = 0;
  }

  return c->negate ? !result : result;
}
