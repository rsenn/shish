#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"
#include <stdlib.h>

/* expand argument vector
 *
 * POSIX field-splitting: an unquoted parameter expansion that produces
 * an empty value contributes NO field. Without this, `exec $CONFIG_SHELL
 * $as_opts "$as_myself"` in autoconf's re-exec passes argv[1] = "" to
 * the new shell, which then complains "/bin/bash: : No such file or
 * directory". Keep empty fields when they came from quoted ("") text.
 * Returns the number of slots written (not counting the terminating
 * NULL); pair with expand_args's return as the canonical argc.
 * ----------------------------------------------------------------------- */
int
expand_argv(union node* args, char** argv) {
  int n = 0;
  for(; args; args = args->next) {
    if(!args->narg.stra.s)
      continue;
    if(args->narg.stra.len == 0 && !(args->narg.flag & (X_QUOTED | X_NOSPLIT)))
      continue;
    *argv++ = args->narg.stra.s;
    n++;
  }

  *argv = NULL;
  return n;
}
