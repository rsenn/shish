#include "../wordlist.h"
#include "../expand.h"
#include "../parse.h"
#include "../../lib/str.h"

static int
is_ifs(const char* ifs, size_t ifslen, char c) {
  return ifslen && str_chr(ifs, c) < ifslen;
}

static int
is_ws(const char* ifs, size_t ifslen, char c) {
  return (c == ' ' || c == '\t' || c == '\n') && is_ifs(ifs, ifslen, c);
}

/* append b[0..len) unescaped: "\\x" -> "x" where parse_isesc(x) */
static void
cat_unescaped(stralloc* sa, const char* b, size_t len) {
  size_t i;

  for(i = 0; i < len; i++) {
    if(b[i] == '\\' && i + 1 < len && parse_isesc(b[i + 1]))
      i++;

    stralloc_catc(sa, b[i]);
  }
}

/* start a sibling of the closed field: both now belong to a split, so neither is dropped */
static void
sibling(wordlist* wl) {
  wl->pend = 0;
  wl->closed = 0;
  wl->state = X_SPLIT;
  wl->cur->len = 0;
}

/* make sure there is an open field to append to */
static void
start(wordlist* wl) {
  if(!wl->has) {
    wl->has = 1;
    wl->closed = 0;
    wl->state = 0;
    wl->cur->len = 0;
  } else if(wl->closed) {
    sibling(wl);
  }
}

/* end the open field: nul, glob or unescape as flags ask, freeze it. Not yet X_SPLIT:
 * a field becomes part of a split once a sibling follows it. */
static void
finish(wordlist* wl, int flags) {
  stralloc_nul(wl->cur);

  if(flags & X_GLOB)
    wordlist_glob(wl, flags & ~X_GLOB);
  else if(flags & X_GLOBRES)
    wordlist_glob(wl, flags);
  else if(flags & X_LITERAL) {
    expand_unescape(wl->cur, parse_isesc);
    wl->state &= ~X_GLOB;
  }

  wordlist_push(wl);
  wl->closed = 1;
}

/* append a chunk to the open field, splitting at IFS when the flags allow it (POSIX 2.6.5).
 *
 *   wordlist*    wl     list to append to
 *   const char*  b      the chunk
 *   size_t       len    its length
 *   int          flags  X_* bits describing where the chunk came from
 * ----------------------------------------------------------------------- */
void
wordlist_cat(wordlist* wl, const char* b, size_t len, int flags) {
  stralloc* cur = wl->cur;
  const char* ifs;
  size_t ifslen, i, start_i;
  int have_field;

  if(wl->ar == NULL)
    flags |= X_NOSPLIT;

  /* no splitting: one field. A closed field is never appended to: a quoted chunk after an
     unquoted chunk's own split ("${p+a "b"}") starts a sibling. */
  if(flags & (X_NOSPLIT | X_QUOTED)) {
    start(wl);

    /* an unquoted literal chunk waiting for its end-of-word unescape must be unescaped now:
       X_UNESCAPED tells that later pass the field is finished ("x\\\\'y'" keeps both backslashes) */
    if((flags & X_LITERAL) && !(flags & X_PATTERN) && (wl->state & X_LITERAL) && !(wl->state & (X_UNESCAPED | X_GLOB)))
      expand_unescape(cur, parse_isesc);

    wl->state |= flags;

    /* a literal chunk and a substitution chunk sharing the field are each unescaped on their
       own, before they meet in cur; X_UNESCAPED marks only chunks that were literal */
    if((flags & X_LITERAL) && !(flags & X_PATTERN)) {
      cat_unescaped(cur, b, len);
      wl->state |= X_UNESCAPED;
    } else if((flags & X_PATTERN) && (flags & X_QUOTED) && !(flags & X_LITERAL)) {
      /* a quoted result inside a pattern is literal text:  "a*"  ->  a\*  */
      for(i = 0; i < len; i++) {
        if(b[i] == '\\' || b[i] == '*' || b[i] == '?' || b[i] == '[')
          stralloc_catc(cur, '\\');

        stralloc_catc(cur, b[i]);
      }
    } else {
      stralloc_catb(cur, b, len);
    }

    return;
  }

  /* a word's own literal text is never split (POSIX 2.6.5); X_SUBWORD excludes the word
     of "${p+word}", whose literal text splits like any expansion result */
  if((flags & X_LITERAL) && !(flags & X_SUBWORD)) {
    start(wl);

    /* a quoted chunk before this one was unescaped on the spot, so the end-of-word pass
       skips the field: this chunk has to be done here */
    if((wl->state & X_UNESCAPED) && !(flags & (X_GLOB | X_PATTERN))) {
      wl->state |= flags;
      cat_unescaped(cur, b, len);
      return;
    }

    wl->state |= flags;
    stralloc_catb(cur, b, len);
    return;
  }

  ifs = wl->ifs ? wl->ifs : "";
  ifslen = str_len(ifs);

  if(len == 0)
    return;

  /* a maximal run of IFS characters is one delimiter:
   *   whitespace only    closes the open field, opens none ("a  b" -> "a","b"); leading or
   *                      trailing it contributes nothing
   *   k non-white chars  k boundaries: each closes the field before it (even empty) and opens
   *                      one after, except the last char of the whole string
   * An empty field is kept when X_SPLIT marks it one of several; see wordlist_push().
   *
   * have_field: a non-empty open field exists to append to. A closed field or a virgin
   * placeholder does not count, so a leading delimiter run is not misread as closing a field. */
  have_field = wl->has && !wl->closed && cur->len > 0;
  start_i = 0;
  i = 0;

  for(;;) {
    while(i < len && !is_ifs(ifs, ifslen, b[i])) {
      if(!have_field) {
        start(wl);
        have_field = 1;
        start_i = i;
      }

      i++;
    }

    if(have_field && i > start_i) {
      wl->state |= flags;
      stralloc_catb(cur, &b[start_i], i - start_i);
    }

    if(i == len)
      break;

    {
      size_t j = i, nws = 0, k;

      while(j < len && is_ifs(ifs, ifslen, b[j])) {
        if(!is_ws(ifs, ifslen, b[j]))
          nws++;

        j++;
      }

      if(nws == 0) {
        /* whitespace run: closes the open field. An empty field that already carries
           X_QUOTED/X_NOSPLIT (the '' in "''$b") is a real one and closes too. */
        if(have_field || (wl->has && !wl->closed && (wl->state & (X_QUOTED | X_NOSPLIT)))) {
          finish(wl, flags);
          have_field = 0;
        }
      } else {
        /* the first non-white char closes whatever is open, real or empty */
        if(!have_field)
          start(wl);

        finish(wl, flags);
        have_field = 0;

        /* each further one closes an empty field of its own */
        for(k = 1; k < nws; k++) {
          sibling(wl);
          finish(wl, flags);
        }
      }

      i = j;
    }
  }
}
