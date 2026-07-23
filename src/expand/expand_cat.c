#include "../expand.h"
#include "../../lib/str.h"
#include "../tree.h"
#include "../var.h"
#include "../parse.h"

#include <stdlib.h>

/* concatenate <len> bytes from <b> to the argument list pointed to by <nptr>
 * ----------------------------------------------------------------------- */
union node*
expand_cat(const char* b, unsigned int len, union node** nptr, int flags) {
  union node* n = *nptr;
  const char* ifs = NULL;
  unsigned int i, x;

  /* if we're not splitting create a new node if there isn't any, even if
     the stralloc has zero length, and concatenate the stralloc as a whole */
  if(flags & (X_NOSPLIT | X_QUOTED)) {
    if(n == NULL) {
      n = *nptr = tree_newnode(N_ARG);
      stralloc_zero(&n->narg.stra);
    }

    n->narg.flag |= flags /*& (~(X_QUOTED))*/;

    /* this branch never splits into fields and never globs, so unlike
       the loop below there's no later point where a deferred,
       whole-buffer expand_unescape() pass could tell a literal
       source-text chunk (parser-doubled, needs unescaping) apart from
       an already-real parameter/command-substitution chunk sharing
       this same node (e.g. an assignment's own "NAME=" prefix next to
       a "$(...)" value) -- the two get concatenated right here and
       any later pass over the combined bytes can only get one of them
       right. Unescaping each chunk immediately, before it ever touches
       the accumulator, keeps the two kinds of bytes from ever being
       confused (assign-cmdsubst-value-loses-escaping, fixes/70).

       X_UNESCAPED is only set when this chunk actually was literal:
       a plain substitution chunk that happens to route through this
       same branch (e.g. quoted "$x") contributes nothing that needs
       correcting, and marking the node anyway would wrongly tell a
       *later*, still-unprocessed chunk sharing this node (e.g. an
       unquoted "\*" tail glued onto "$x" with no separator, still
       headed for the split loop below on its own call) that it's
       already been handled when it hasn't. */
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

  ifs = var_vdefault("IFS", IFS_DEFAULT, NULL);

  for(i = 0;;) {
    /* skip field separators */
    for(x = 0; i < len; i++, x++) {
      if(ifs[str_chr(ifs, b[i])] == '\0')
        break;
    }

    /* finished */
    if(i == len)
      break;

    /* if there isn't already a node create one now! */
    if(n == NULL) {
      *nptr = n = tree_newnode(N_ARG);
      nptr = &n;
      stralloc_init(&n->narg.stra);
    }
    /* if there were separators delimit the current field by creating a new node
     */
    else if(x) {
      stralloc_nul(&n->narg.stra);

      if(flags & X_GLOB) {
        if((n = expand_glob(nptr, flags & ~X_GLOB)))
          nptr = &n;
      } else if(flags & X_LITERAL) {
        expand_unescape(&n->narg.stra, parse_isesc);
        n->narg.flag &= ~X_GLOB;
      }

      n->next = tree_newnode(N_ARG);
      n = n->next;
      stralloc_init(&n->narg.stra);
    }

    /* skip non-separators */
    for(x = 0; i < len; i++, x++)
      if(ifs[str_chr(ifs, b[i])])
        break;

    /* there were non-separators: fill the stralloc of the current argument node
     *
     * plain copy, not expand_escape(): the raw text here already has
     * exactly the right shape for glob(3) to interpret -- an
     * unescaped "*"/"?"/"[" is a real wildcard the user wrote bare,
     * and a "\*"/"\?"/"\[" the user backslash-escaped is *already*
     * glob(3)'s own escape syntax for a literal char, preserved
     * as-is by parse_unquoted.c's own backslash handling (it only
     * strips the backslash for non-glob-special chars). Blanket
     * re-escaping every occurrence here (as expand_escape() does)
     * turned every bare wildcard into an escaped literal, so no glob
     * pattern was ever actually reachable from a plain command
     * argument (glob-not-triggered-for-plain-arguments, fixes/66). */
    n->narg.flag |= flags;
    stralloc_catb(&n->narg.stra, &b[i - x], x);

    /* finished */
    if(i == len)
      break;
  }

  return n;
}
