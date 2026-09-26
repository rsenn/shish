#include "sed_internal.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"
#include "../../lib/scan.h"
#include "../../lib/stralloc.h"

/* sed_subst_parse: 's<delim>pattern<delim>replacement<delim>flags'.
 * Pattern and replacement keep every backslash sequence except
 * '\<delim>' (a literal delimiter) and, in the pattern only,
 * '\<newline>' (a literal newline); the regex and replacement-
 * template compilers interpret everything else. Flags: a decimal N,
 * 'g', 'p', 'i'/'I', 'w file' (must come last: the rest of the line
 * is the file name).
 * ----------------------------------------------------------------------- */
int
sed_subst_parse(
    const char** pp, const char* end, struct sed_subst* s, unsigned flags, struct sed* prog) {
  const char* p = *pp;
  char delim;
  stralloc pat, repl;
  const char* start;
  int rc = SED_OK;
  unsigned dfaflags;

  byte_zero(s, sizeof(*s));
  s->wfile = -1;

  if(p >= end)
    return SED_EDELIM;

  delim = *p;

  if(delim == '\\' || delim == '\n')
    return SED_EDELIM;

  p++;
  stralloc_init(&pat);
  stralloc_init(&repl);

  start = p;

  while(p < end && *p != delim) {
    /* '\<delim>' and '\<real newline>' insert that byte literally;
       '\n' (backslash + the *letter* n) is undefined by POSIX BRE/ERE
       and every real sed treats it as a newline escape, so an
       embedded newline from 'N' can be matched on one script line
       ("N;s/\n/ /") instead of only via an actual multi-line pattern. */
    if(*p == '\\' && p + 1 < end && (p[1] == delim || p[1] == '\n' || p[1] == 'n')) {
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
    } else if(*p == '\n') {
      rc = SED_EUNTERM;
      goto done;
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
  start = p;

  while(p < end && *p != delim) {
    if(*p == '\\' && p + 1 < end)
      p += 2;
    else if(*p == '\n') {
      rc = SED_EUNTERM;
      goto done;
    } else
      p++;
  }

  if(p >= end) {
    rc = SED_EUNTERM;
    goto done;
  }

  if(!stralloc_catb(&repl, start, (size_t)(p - start))) {
    rc = SED_ENOMEM;
    goto done;
  }

  p++;

  for(;;) {
    if(p < end && *p >= '0' && *p <= '9') {
      unsigned long v;
      size_t used = scan_ulong(p, &v);

      s->nth = (unsigned)v;
      p += used;
    } else if(p < end && *p == 'g') {
      s->global = 1;
      p++;
    } else if(p < end && *p == 'p') {
      s->print = 1;
      p++;
    } else if(p < end && (*p == 'i' || *p == 'I')) {
      s->icase = 1;
      p++;
    } else if(p < end && *p == 'w') {
      const char* name;
      int idx;

      p++;

      while(p < end && (*p == ' ' || *p == '\t'))
        p++;

      name = p;

      while(p < end && *p != '\n')
        p++;

      if(p == name) {
        rc = SED_EUNTERM;
        goto done;
      }

      idx = sed_wfile_intern(prog, name, (size_t)(p - name));

      if(idx < 0) {
        rc = SED_ENOMEM;
        goto done;
      }

      s->wfile = idx;
      break;
    } else {
      break;
    }
  }

  dfaflags = (flags & SED_ERE) ? DFA_ERE : 0;

  if(s->icase)
    dfaflags |= DFA_ICASE;

  if(pat.len == 0) {
    s->re_set = 0;
  } else if(dfa_compile(&s->re, pat.s, pat.len, dfaflags) != DFA_OK) {
    rc = SED_EREGEX;
    goto done;
  } else {
    s->re_set = 1;
  }

  if(dfa_repl_compile(&s->repl, repl.s ? repl.s : "", repl.len, DFA_REPL_BACKREF) != DFA_OK) {
    rc = SED_ENOMEM;
    goto done;
  }

  *pp = p;

done:
  stralloc_free(&pat);
  stralloc_free(&repl);

  if(rc != SED_OK && s->re_set) {
    dfa_free(&s->re);
    s->re_set = 0;
  }

  return rc;
}

void
sed_subst_free(struct sed_subst* s) {
  if(s->re_set)
    dfa_free(&s->re);

  dfa_repl_free(s->repl);
}

static int
subst_append(void* ctx, const char* s, size_t n) {
  return !stralloc_catb((stralloc*)ctx, s, n);
}

/* sed_subst_exec: applies s to st->pattern in place. Returns 1 if a
 * replacement was made (and sets st->tflag), 0 otherwise (including
 * an empty // with no prior regex, or out of memory -- both leave
 * the pattern space untouched rather than erroring out mid-run).
 * ----------------------------------------------------------------------- */
int
sed_subst_exec(struct sed_subst* s, struct sed_state* st) {
  struct dfa* re = s->re_set ? &s->re : st->last_re;
  stralloc result;
  unsigned nreplaced = 0;
  int rc;

  if(!re)
    return 0;

  stralloc_init(&result);
  rc = dfa_replace(re,
                   s->repl,
                   st->pattern.s,
                   st->pattern.len,
                   s->nth,
                   s->global,
                   subst_append,
                   &result,
                   &nreplaced);
  st->last_re = re;

  if(rc || nreplaced == 0) {
    stralloc_free(&result);
    return 0;
  }

  stralloc_free(&st->pattern);
  st->pattern = result;
  st->tflag = 1;
  return 1;
}

/* sed_y_parse: 'y<delim>from<delim>to<delim>', from and to the same
 * length after unescaping ('\<delim>', '\\', '\n' recognized; any
 * other backslash sequence keeps the escaped character literally).
 * ----------------------------------------------------------------------- */
static int
y_list(const char** pp, const char* end, char delim, stralloc* out) {
  const char* p = *pp;

  while(p < end && *p != delim) {
    if(*p == '\\' && p + 1 < end) {
      unsigned char lit;

      if(p[1] == delim)
        lit = (unsigned char)delim;
      else if(p[1] == '\\')
        lit = '\\';
      else if(p[1] == 'n')
        lit = '\n';
      else
        lit = (unsigned char)p[1];

      if(!stralloc_catc(out, lit))
        return SED_ENOMEM;

      p += 2;
    } else if(*p == '\n') {
      return SED_EUNTERM;
    } else {
      if(!stralloc_catc(out, (unsigned char)*p))
        return SED_ENOMEM;

      p++;
    }
  }

  if(p >= end)
    return SED_EUNTERM;

  p++;
  *pp = p;
  return SED_OK;
}

int
sed_y_parse(const char** pp, const char* end, unsigned char map[256]) {
  const char* p = *pp;
  char delim;
  stralloc from, to;
  int rc;
  size_t i;

  if(p >= end)
    return SED_EDELIM;

  delim = *p;

  if(delim == '\\' || delim == '\n')
    return SED_EDELIM;

  p++;
  stralloc_init(&from);
  stralloc_init(&to);

  for(i = 0; i < 256; i++)
    map[i] = (unsigned char)i;

  if((rc = y_list(&p, end, delim, &from)) != SED_OK)
    goto done;

  if((rc = y_list(&p, end, delim, &to)) != SED_OK)
    goto done;

  if(from.len != to.len) {
    rc = SED_EDELIM;
    goto done;
  }

  for(i = 0; i < from.len; i++)
    map[(unsigned char)from.s[i]] = (unsigned char)to.s[i];

  *pp = p;
  rc = SED_OK;

done:
  stralloc_free(&from);
  stralloc_free(&to);
  return rc;
}
