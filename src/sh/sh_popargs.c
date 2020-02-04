#include "../sh.h"

void
sh_popargs(struct arg* arg) {
  sh->arg.c = arg->c;
  sh->arg.v = arg->v;
  sh->arg.a = arg->a;
  sh->arg.s = arg->s;
}
