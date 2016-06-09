#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glob.h>

#if defined(__MINGW32__) || defined(__MINGW64__)
#include "mingw-glob.h"
#endif

#include <stdlib.h>
#include "var.h"
#include "tree.h"
#include "expand.h"

/* perform glob() expansion on the current argument
 * ----------------------------------------------------------------------- */
union node *expand_glob(union node **nptr, int flags) {
  union node *n;
  glob_t glb;
  int ret;
  const char *ifs = var_vdefault("IFS", IFS_DEFAULT, NULL);

  if(!(n = *nptr))
    return n;

  stralloc_nul(&n->narg.stra);

  /* glob for the pattern */
#ifdef HAVE_GLOB
  if(!(ret = glob(n->narg.stra.s, 0, NULL, &glb))) {
    unsigned int i;

    /* got some result, clear current argument string */
    stralloc_zero(&n->narg.stra);

    /* loop through expanded paths */
    for(i = 0; i < glb.gl_pathc;) {
      stralloc_cats(&n->narg.stra, glb.gl_pathv[i]);

      /* if there is another path then delimit the current one */
      if(++i < glb.gl_pathc) {
        /* if we should not split then just concat ifs[0] */
        if(flags & X_NOSPLIT) {
          stralloc_catc(&n->narg.stra, ifs[0]);
        }
        /* otherwise create a new node */
        else {
          n->list.next = tree_newnode(N_ARG);
          n = n->list.next;
          stralloc_init(&n->narg.stra);
        }

        n->narg.flag |= flags;
      }
    }
    
    globfree(&glb);
  } else
#endif
  {
    expand_unescape(&n->narg.stra);
  }

  return n;
}

