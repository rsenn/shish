#include "shell.h"
#include "debug.h"
#include "sh.h"

/* set arguments of the current shell env
 * FIXME: do we always need to strdup() them?
 * ----------------------------------------------------------------------- */
void sh_setargs(char **argv, int dup) {
  int i;

  sh->arg.v -= sh->arg.s;
  sh->arg.c += sh->arg.s;
  sh->arg.s = 0;

  /* free current argv if it is not the initial one */
  if(sh->arg.v != sh_argv && sh->arg.a) {
    for(i = 0; sh->arg.v[i]; i++)
      shell_free(sh->arg.v[i]);

    shell_free(sh->arg.v);
  }

  if(argv == NULL)
    return;

  /* set new args */
  if(argv == sh_argv || !dup) {
    sh->arg.v = sh_argv;
    sh->arg.c = sh_argc;
    sh->arg.a = 0;
  } else {
    for(sh->arg.c = 0; argv[sh->arg.c]; sh->arg.c++);

    sh->arg.v = shell_alloc(sizeof(char *) * (sh->arg.c + 1));

    for(i = 0; i < sh->arg.c; i++)
      sh->arg.v[i] = shell_strdup(argv[i]);

    sh->arg.v[i] = NULL;
    sh->arg.a = 1;
  }
}
