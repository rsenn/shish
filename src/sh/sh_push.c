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

  /* copy cwd */
  /*  stralloc_copyb(&env->cwd, sh->cwd.s, sh->cwd.len);*/
  env->cwd.a = 0;
  env->cwd.s = sh->cwd.s;
  env->cwd.len = sh->cwd.len;

  env->cwdsym = sh->cwdsym;

  env->umask = sh->umask;
  env->exitcode = 0;

  env->arg.c = sh->arg.c;
  env->arg.v = sh->arg.v;
  env->arg.s = sh->arg.s;
  env->arg.a = 0;

  env->eval = NULL;
  env->fdstack = fdstack;
  env->varstack = varstack;

  sh = env;
}
