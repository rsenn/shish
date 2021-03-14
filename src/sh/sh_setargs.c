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
    for(i = 0; args->v[i]; i++) shell_free(args->v[i]);

    shell_free(args->v);
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

    args->v = shell_alloc(sizeof(char*) * (args->c + 1));

    for(i = 0; i < args->c; i++) args->v[i] = shell_strdup(argv[i]);

    args->v[i] = NULL;
    args->a = 1;
  }
}
