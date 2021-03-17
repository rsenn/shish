#include "../../lib/byte.h"
#include "../tree.h"
#include <stdlib.h>

/* expand argument vector
 * ----------------------------------------------------------------------- */
void
expand_argv(union node* args, char** argv) {
  for(; args; args = args->narg.next) {
    if(args->narg.stra.s)
      *argv++ = args->narg.stra.s;
  }

  *argv = NULL;
}
