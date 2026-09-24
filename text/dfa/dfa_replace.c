#include "dfa_prog.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

/* struct dfa_repl (a compiled replacement template: "&"/\1-\9 group
 * references interleaved with literal runs) and dfa_repl_apply (its
 * one-match expansion) live here, private to dfa_replace() -- the
 * only caller, since dfa_replace's nth/global already cover a single
 * first-match substitution (nth=1, global=0, awk's future sub()) as
 * well as sed's s///g, so nothing else needs to apply a template on
 * its own.
 * ----------------------------------------------------------------------- */
struct dfa_repl_item {
  int group; /* -1 = literal run (see off/len below); 0 = "&"; 1-9 = "\N" */
  size_t off, len;
};

struct dfa_repl {
  struct dfa_repl_item* items;
  size_t nitems, cap;
  char* lit;
  size_t litlen, litcap;
};

static int
repl_addliteral(struct dfa_repl* r, const char* s, size_t len) {
  if(len == 0)
    return DFA_OK;

  if(r->litlen + len > r->litcap) {
    size_t ncap = r->litcap ? r->litcap * 2 : 32;
    char* nl;

    while(ncap < r->litlen + len)
      ncap *= 2;

    nl = alloc_re(r->lit, ncap);

    if(!nl)
      return DFA_ENOMEM;

    r->lit = nl;
    r->litcap = ncap;
  }

  byte_copy(r->lit + r->litlen, len, s);

  /* coalesce with a directly-preceding literal run instead of adding
     a new item, so e.g. "\&x" (an escaped '&' followed by a plain
     'x') ends up as one literal item, not two. */
  if(r->nitems && r->items[r->nitems - 1].group < 0
     && r->items[r->nitems - 1].off + r->items[r->nitems - 1].len == r->litlen) {
    r->items[r->nitems - 1].len += len;
  } else {
    if(r->nitems == r->cap) {
      size_t ncap = r->cap ? r->cap * 2 : 8;
      struct dfa_repl_item* ni = alloc_re(r->items, ncap * sizeof(*ni));

      if(!ni)
        return DFA_ENOMEM;

      r->items = ni;
      r->cap = ncap;
    }

    r->items[r->nitems].group = -1;
    r->items[r->nitems].off = r->litlen;
    r->items[r->nitems].len = len;
    r->nitems++;
  }

  r->litlen += len;
  return DFA_OK;
}

static int
repl_addgroup(struct dfa_repl* r, int group) {
  if(r->nitems == r->cap) {
    size_t ncap = r->cap ? r->cap * 2 : 8;
    struct dfa_repl_item* ni = alloc_re(r->items, ncap * sizeof(*ni));

    if(!ni)
      return DFA_ENOMEM;

    r->items = ni;
    r->cap = ncap;
  }

  r->items[r->nitems].group = group;
  r->items[r->nitems].off = 0;
  r->items[r->nitems].len = 0;
  r->nitems++;
  return DFA_OK;
}

/* dfa_repl_compile: '&' -> group 0 (whole match); '\1'-'\9' -> that
 * group (only with DFA_REPL_BACKREF); '\&' '\\' -> literal '&' '\';
 * with DFA_REPL_BACKREF, '\n' -> a literal newline; any other '\X'
 * drops the backslash and keeps X literal (undefined by POSIX; this
 * matches GNU/BSD sed).
 * ----------------------------------------------------------------------- */
int
dfa_repl_compile(struct dfa_repl** out, const char* tmpl, size_t len, unsigned flags) {
  struct dfa_repl* r;
  const char* p = tmpl;
  const char* end = tmpl + len;
  int rc = DFA_OK;

  *out = NULL;
  r = alloc(sizeof(*r));

  if(!r)
    return DFA_ENOMEM;

  byte_zero(r, sizeof(*r));

  while(p < end && rc == DFA_OK) {
    if(*p == '&') {
      rc = repl_addgroup(r, 0);
      p++;
    } else if(*p == '\\' && p + 1 < end) {
      char c = p[1];

      if((flags & DFA_REPL_BACKREF) && c >= '1' && c <= '9') {
        rc = repl_addgroup(r, c - '0');
        p += 2;
      } else if(c == '&' || c == '\\') {
        rc = repl_addliteral(r, &c, 1);
        p += 2;
      } else if((flags & DFA_REPL_BACKREF) && c == 'n') {
        char nl = '\n';

        rc = repl_addliteral(r, &nl, 1);
        p += 2;
      } else {
        rc = repl_addliteral(r, &c, 1);
        p += 2;
      }
    } else {
      const char* start = p;

      while(p < end && *p != '&' && *p != '\\')
        p++;

      rc = repl_addliteral(r, start, (size_t)(p - start));
    }
  }

  if(rc != DFA_OK) {
    dfa_repl_free(r);
    return rc;
  }

  *out = r;
  return DFA_OK;
}

void
dfa_repl_free(struct dfa_repl* r) {
  if(!r)
    return;

  alloc_free(r->items);
  alloc_free(r->lit);
  alloc_free(r);
}

static int
dfa_repl_apply(const struct dfa_repl* r, const char* s, const struct dfa_span* m,
               const struct dfa_span* g, size_t ng, dfa_repl_out_fn out, void* ctx) {
  size_t i;

  for(i = 0; i < r->nitems; i++) {
    const struct dfa_repl_item* it = &r->items[i];

    if(it->group < 0) {
      if(out(ctx, r->lit + it->off, it->len))
        return -1;
    } else if(it->group == 0) {
      if(out(ctx, s + m->start, m->end - m->start))
        return -1;
    } else {
      size_t gi = (size_t)(it->group - 1);

      if(gi < ng && g[gi].start != (size_t)-1) {
        if(out(ctx, s + g[gi].start, g[gi].end - g[gi].start))
          return -1;
      }
    }
  }

  return 0;
}

int
dfa_replace(struct dfa* d, const struct dfa_repl* r, const char* s, size_t n, unsigned nth,
            int global, dfa_repl_out_fn out, void* ctx, unsigned* nreplaced) {
  size_t pos = 0;
  unsigned count = 0;
  unsigned matched = 0;
  struct dfa_span m;
  struct dfa_span g[9];
  size_t ng = dfa_groups(d);

  if(ng > 9)
    ng = 9;

  if(nth == 0)
    nth = 1;

  while(pos <= n) {
    if(!dfa_search(d, s, n, pos, &m))
      break;

    matched++;

    if(matched < nth) {
      /* not yet at the nth match: copy through it unchanged (an
         empty match still needs a one-byte nudge so the next search
         makes progress) and keep looking */
      size_t upto = m.end > m.start ? m.end : (m.start < n ? m.start + 1 : m.start);

      if(out(ctx, s + pos, upto - pos))
        return -1;

      pos = upto;

      if(m.start == m.end && m.start >= n)
        break;

      continue;
    }

    if(out(ctx, s + pos, m.start - pos))
      return -1;

    if(ng)
      dfa_submatch(d, s, n, &m, g, ng);

    if(dfa_repl_apply(r, s, &m, g, ng, out, ctx))
      return -1;

    count++;
    pos = m.end;

    if(!global)
      break;

    if(m.start == m.end) {
      if(pos < n) {
        if(out(ctx, s + pos, 1))
          return -1;

        pos++;
      } else {
        break;
      }
    }
  }

  if(out(ctx, s + pos, n - pos))
    return -1;

  if(nreplaced)
    *nreplaced = count;

  return 0;
}
