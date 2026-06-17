#include "../sh.h"
#include "../../lib/byte.h"
#include "../fdstack.h"
#include "../vartab.h"

/* pushes current shell environment and creates new one
 * ----------------------------------------------------------------------- */
void
sh_push(struct env* env) {
  byte_zero(env, sizeof(struct env));

  env->parent = sh;

  /* Own a private cwd copy. Aliasing sh->cwd.s here without taking
     ownership (the previous behavior) means any stralloc reallocation
     of the parent's cwd -- which happens on cd, getcwd refresh, etc. --
     leaves this env's cwd.s dangling, and sh_pop later dereferences it
     in stralloc_diffs. autoconf's nested-subshell shell-detection probe
     triggered this within ~140 lines of `./configure` start. */
  stralloc_init(&env->cwd);
  if(sh->cwd.len)
    stralloc_copyb(&env->cwd, sh->cwd.s, sh->cwd.len);

  env->cwdsym = sh->cwdsym;

  env->opts = sh->opts;
  env->umask = sh->umask;
  env->exitcode = 0;

  /*env->arg.c = sh->arg.c;
  env->arg.v = sh->arg.v;
  env->arg.s = sh->arg.s;
  env->arg.a = 0;*/
  sh_pushargs(&env->arg);

  env->eval = NULL;
  env->fdstack = fdstack;
  env->varstack = varstack;

  sh = env;
}
