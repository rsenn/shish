#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"
#include <stdlib.h>

/* expand argument vector.
 *
 * POSIX field-splitting: an unquoted parameter expansion that produces
 * an empty value contributes no field. Drop such empty fields, but
 * keep an empty field when:
 * - it came from quoted ("") text, or
 * - it's one of 2+ real fields splitting produced from one word
 *   (X_SPLIT, see expand.h), e.g. IFS="=" on "a==" giving two
 *   genuinely distinct "" fields, rather than a word's sole result
 *   collapsing to nothing.
 *
 * Returns the number of slots written (not counting the terminating
 * NULL); pair with expand_args's return as the canonical argc.
 *
 *   union node*  args   expanded N_ARG chain to flatten
 *   char**       argv   destination array, NULL-terminated on return
 * ----------------------------------------------------------------------- */
int
expand_argv(union node* args, char** argv) {
  int n = 0;
  for(; args; args = args->next) {
    if(!args->narg.stra.s)
      continue;
    if(args->narg.stra.len == 0 && !(args->narg.flag & (X_QUOTED | X_NOSPLIT | X_SPLIT)))
      continue;
    *argv++ = args->narg.stra.s;
    n++;
  }

  *argv = NULL;
  return n;
}
