#include "byte.h"
#include "sh.h"
#include "eval.h"
#include "fdstack.h"
#include "vartab.h"

/*struct eval *eval = NULL;*/

void eval_push(struct eval *e, int flags) {
  byte_zero(e, sizeof(struct eval));
  e->flags = flags;
  e->parent = sh->eval;
  
  /* remember stack locations in current nesting level */
  e->fdstack = fdstack;
  e->varstack = varstack;
  
  sh->eval = e;
}

              
