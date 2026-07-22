#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* lib/unix/glob.c -- the project's own glob()/globfree() -- only
 * actually compiles in on Windows (its whole body is guarded by "#if
 * WINDOWS_NATIVE"); everywhere else HAVE_GLOB means the platform's
 * real libc glob() gets linked instead. glob_t's layout has to come
 * from whichever one that is: the system <glob.h> struct is
 * considerably larger than lib/glob.h's own (glibc's, for instance,
 * adds several GLOB_ALTDIRFUNC callback members) -- unconditionally
 * using the small lib/glob.h struct here while HAVE_GLOB silently
 * links the *system*'s glob() made it write past the end of a
 * too-small on-stack glob_t on every normal (non-Windows) build,
 * corrupting the stack on any command using a glob pattern
 * (confirmed via AddressSanitizer, expand-glob-stack-overflow). */
#ifdef HAVE_GLOB
#include <glob.h>
#else
#include "../../lib/glob.h"
#endif

#include "../expand.h"
#include "../tree.h"
#include "../fdtable.h"
#include "../var.h"
#include "../parse.h"
#include "../../lib/byte.h"
#include <stdlib.h>
#include <string.h>

int
expand_globerr(const char* epath, int eerrno) {
  buffer_puts(fd_err->w, "glob error on: ");
  buffer_puts(fd_err->w, epath);
  buffer_puts(fd_err->w, " ");
  buffer_puts(fd_err->w, strerror(eerrno));
  buffer_putnlflush(fd_err->w);
  return 0;
}

/* perform glob() expansion on the current argument
 * ----------------------------------------------------------------------- */
union node*
expand_glob(union node** nptr, int flags) {
  union node* n;
  glob_t glb;
  int ret;
  const char* ifs = var_vdefault("IFS", IFS_DEFAULT, NULL);

  if(!(n = *nptr))
    return n;

  stralloc_nul(&n->narg.stra);

  /* glob for the pattern */
#ifdef HAVE_GLOB
  byte_zero(&glb, sizeof(glb));

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
          n->next = tree_newnode(N_ARG);
          n = n->next;
          stralloc_init(&n->narg.stra);
        }

        n->narg.flag |= flags;
      }
    }

    globfree(&glb);
  } else
#endif
  {
    expand_unescape(&n->narg.stra, parse_isesc);
  }

  return n;
}
