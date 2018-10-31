#include <stdlib.h>
#include "byte.h"
#include "tree.h"

/* expand argument vector
 * ----------------------------------------------------------------------- */
void expand_argv(union node *args, char **argv) {
  for(; args; args = args->list.next) {
    if(args->narg.stra.s)
      *argv++ = args->narg.stra.s;
  }
  
  *argv = NULL;
}


