#include "../../lib/alloc.h"
#include "../debug.h"
#include "../sh.h"
#include "../../lib/shell.h"

/* set arguments of the current shell env
 * FIXME: do we always need to strdup() them?
 * ----------------------------------------------------------------------- */
void
sh_setargs(char** argv, int dup) {
  unsigned i;
  struct arg* args = &sh->arg;

  args->v -= args->s;
  args->c += args->s;
  args->s = 0;

  /* free current argv if it is not the initial one */
  if(args->v != sh_argv && args->a) {
    for(i = 0; args->v[i]; i++)
      alloc_free(args->v[i]);

    alloc_free(args->v);
  }

  if(argv == NULL)
    return;

  /* set new args */
  if(argv == sh_argv || !dup) {
    args->v = sh_argv;
    args->c = sh_argc;
    args->a = 0;
  } else {
    for(args->c = 0; argv[args->c]; args->c++)
      ;

    args->v = alloc(sizeof(char*) * (args->c + 1));

    for(i = 0; i < args->c; i++)
      args->v[i] = str_dup(argv[i]);

    args->v[args->c] = NULL;
    args->a = args->c + 1;
  }
}
