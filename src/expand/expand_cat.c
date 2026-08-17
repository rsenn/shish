#include "../expand.h"
#include "../../lib/str.h"
#include "../tree.h"
#include "../var.h"
#include "../parse.h"

#include <stdlib.h>

static int
ifs_is_ifs(const char* ifs, size_t ifslen, char c) {
  return ifslen && str_chr(ifs, c) < ifslen;
}

static int
ifs_is_ws(const char* ifs, size_t ifslen, char c) {
  return (c == ' ' || c == '\t' || c == '\n') && ifs_is_ifs(ifs, ifslen, c);
}

/* finalize field node *np: nul-terminate it and run whatever
 * glob/unescape pass its flags call for. Does NOT mark X_SPLIT -- a
 * field only becomes part of a split once a second field is linked
 * after it, see expand_cat_sibling() below.
 * ----------------------------------------------------------------------- */
static void
expand_cat_finish(union node** np, int flags) {
  union node* n = *np;

  stralloc_nul(&n->narg.stra);

  if(flags & X_GLOB) {
    union node* g = expand_glob(np, flags & ~X_GLOB);

    if(g)
      n = *np = g;
  } else if(flags & X_LITERAL) {
    expand_unescape(&n->narg.stra, parse_isesc);
    n->narg.flag &= ~X_GLOB;
  }

  n->narg.flag |= X_CATCLOSED;
}

/* link a fresh, empty field after the already-closed field *np, mark
 * both of them X_SPLIT (this word now genuinely has 2+ fields, so
 * neither may be dropped later even if it turns out empty -- see
 * X_SPLIT's own comment in expand.h), and move *np to the new node.
 * ----------------------------------------------------------------------- */
static void
expand_cat_sibling(union node** np) {
  union node* n = *np;

  n->narg.flag |= X_SPLIT;
  n->next = tree_newnode(N_ARG);
  n = n->next;
  stralloc_init(&n->narg.stra);
  n->narg.flag |= X_SPLIT;
  *np = n;
}

/* concatenate <len> bytes from <b> to the argument list pointed to by <nptr>
 * ----------------------------------------------------------------------- */
union node*
expand_cat(const char* b, unsigned int len, union node** nptr, int flags) {
  union node* n = *nptr;
  const char* ifs = NULL;
  size_t ifslen;
  unsigned int i, start;
  int have_field;

  /* if we're not splitting create a new node if there isn't any, even if
     the stralloc has zero length, and concatenate the stralloc as a whole.
     A closed field (X_CATCLOSED) must not be appended onto: it was closed
     by an earlier unquoted chunk's own splitting within the same word
     (e.g. the literal space between two quoted strings inside a
     "${parameter+word}" operator's word), so this quoted chunk needs a
     fresh sibling field instead of silently merging back into it. */
  if(flags & (X_NOSPLIT | X_QUOTED)) {
    if(n == NULL) {
      n = *nptr = tree_newnode(N_ARG);
      stralloc_zero(&n->narg.stra);
    } else if(n->narg.flag & X_CATCLOSED) {
      expand_cat_sibling(&n);
    }

    n->narg.flag |= flags /*& (~(X_QUOTED))*/;

    /* This branch never splits or globs, so a literal chunk (parser-
     * doubled, needs unescaping) and an already-real substitution
     * chunk sharing this node (e.g. "NAME=" next to a "$(...)" value)
     * must each be unescaped immediately, before touching the shared
     * accumulator -- a later whole-buffer pass couldn't tell them
     * apart once concatenated.
     *
     * X_UNESCAPED marks only chunks that were actually literal, so a
     * later chunk sharing this node isn't wrongly treated as already
     * handled. */
    if((flags & X_LITERAL) && !(flags & X_PATTERN)) {
      stralloc tmp;
      stralloc_init(&tmp);
      stralloc_catb(&tmp, b, len);
      expand_unescape(&tmp, parse_isesc);
      stralloc_catb(&n->narg.stra, tmp.s, tmp.len);
      stralloc_free(&tmp);
      n->narg.flag |= X_UNESCAPED;
    } else {
      stralloc_catb(&n->narg.stra, b, len);
    }

    return n;
  }

  /* A word's own literal text is never subject to field splitting
   * (POSIX 2.6.5: splitting applies only to expansion results).
   * X_SUBWORD excludes this: literal text inside a
   * "${parameter+word}"-style operator's nested word isn't its own
   * word, so it splits along with the rest of the operator's result. */
  if((flags & X_LITERAL) && !(flags & X_SUBWORD)) {
    if(n == NULL || (n->narg.flag & X_CATCLOSED)) {
      if(n == NULL) {
        *nptr = n = tree_newnode(N_ARG);
        stralloc_init(&n->narg.stra);
        nptr = &n;
      } else {
        expand_cat_sibling(&n);
      }
    }

    n->narg.flag |= flags;
    stralloc_catb(&n->narg.stra, b, len);
    return n;
  }

  ifs = var_vdefault("IFS", IFS_DEFAULT, NULL);
  ifslen = str_len(ifs);

  if(len == 0)
    return n;

  /* POSIX 2.6.5 field splitting: a maximal run of IFS characters is a
   * "delimiter".
   * - A pure-whitespace run closes the open field and opens no field
   *   of its own ("a  b" -> "a","b"), unless it's leading/trailing,
   *   where it contributes nothing at all.
   * - A run with k >= 1 non-whitespace IFS chars produces k
   *   boundaries: each closes the field before it (even if empty) and
   *   opens a new one after, except the last char consumed in the
   *   whole string, which never opens a trailing field.
   * - A resulting empty field is not dropped here -- X_SPLIT marks it
   *   so expand_argv.c can tell "one of several real fields" apart
   *   from "the word's sole, empty result".
   *
   * (have_field, start) track a field being accumulated into `n`.
   * have_field requires n to be non-NULL, not X_CATCLOSED, and
   * nonempty -- excluding both a closed field and a virgin,
   * zero-length placeholder node expand_args.c may have pre-created
   * for the next word. That keeps a leading delimiter run in this
   * word's own value from being misread as closing an already-open
   * field (which would insert a spurious empty field between two
   * words). Either way, `n` stays reusable in place the moment real
   * content or a delimiter is found. */
  have_field = (n != NULL) && !(n->narg.flag & X_CATCLOSED) && n->narg.stra.len > 0;
  start = 0;
  i = 0;

  for(;;) {
    while(i < len && !ifs_is_ifs(ifs, ifslen, b[i])) {
      if(!have_field) {
        if(n == NULL) {
          *nptr = n = tree_newnode(N_ARG);
          stralloc_init(&n->narg.stra);
          nptr = &n;
        } else if(n->narg.flag & X_CATCLOSED) {
          expand_cat_sibling(&n);
        }
        /* else: n is a virgin, reusable node -- use it directly */

        have_field = 1;
        start = i;
      }

      i++;
    }

    if(have_field && i > start) {
      /* plain copy, not expand_escape(): the raw text already has
         exactly the right shape for glob(3) to interpret */
      n->narg.flag |= flags;
      stralloc_catb(&n->narg.stra, &b[start], i - start);
    }

    if(i == len)
      break;

    {
      unsigned int j = i;
      unsigned int nws_count = 0, k;

      while(j < len && ifs_is_ifs(ifs, ifslen, b[j])) {
        if(!ifs_is_ws(ifs, ifslen, b[j]))
          nws_count++;

        j++;
      }

      if(nws_count == 0) {
        /* pure-whitespace run: closes whatever field is open (nothing
           to close at the start/end of the string), never opens a
           field of its own. A field also counts as open when it's
           empty but already carries X_QUOTED/X_NOSPLIT from an
           earlier chunk of this same word (e.g. the '' in "''$b"):
           such a field is a real, deliberately-empty field, not an
           unused virgin placeholder, and must not be left open for
           a later chunk to silently merge into. */
        if(have_field || (n != NULL && !(n->narg.flag & X_CATCLOSED) &&
                           (n->narg.flag & (X_QUOTED | X_NOSPLIT)))) {
          expand_cat_finish(&n, flags);
          have_field = 0;
        }
      } else {
        /* first non-whitespace IFS char closes whatever's open (real
           or empty); each additional one closes an empty field of its
           own. */
        if(have_field) {
          expand_cat_finish(&n, flags);
        } else if(n != NULL && (n->narg.flag & X_CATCLOSED)) {
          expand_cat_sibling(&n);
          expand_cat_finish(&n, flags);
        } else if(n != NULL) {
          /* virgin, reusable node: it becomes this word's own empty
             leading field directly, no sibling needed */
          expand_cat_finish(&n, flags);
        } else {
          *nptr = n = tree_newnode(N_ARG);
          stralloc_init(&n->narg.stra);
          nptr = &n;
          expand_cat_finish(&n, flags);
        }

        have_field = 0;

        for(k = 1; k < nws_count; k++) {
          expand_cat_sibling(&n);
          expand_cat_finish(&n, flags);
        }
      }

      i = j;
    }
  }

  return n;
}
