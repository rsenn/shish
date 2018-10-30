#include "sh.h"

void sh_pushargs(struct arg *arg) {
  arg->c = sh->arg.c;
  arg->v = sh->arg.v;
  arg->a = sh->arg.a;
  arg->s = sh->arg.s;
  
  sh->arg.a = 0;
}
