#include "../trace.h"
#include "../sh.h"

void
sh_pushargs(struct arg* arg) {
  arg->c = sh->arg.c;
  arg->v = sh->arg.v;
  // arg->a = sh->arg.a;
  arg->s = sh->arg.s;

  arg->a = 0;

  TRACE(TRACE_SH, "pushargs", trace_int("argc", sh->arg.c));
}
