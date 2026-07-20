#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../source.h"
#include "../vartab.h"
#include <assert.h>

int
eval_pop(struct eval* e) {
  int ret;

  assert(e == eval);

  ret = e->exitcode;

  while(fdstack != e->fdstack && &fdstack_root != fdstack)
    fdstack_pop(fdstack);

  while(varstack != e->varstack && &vartab_root != varstack)
    vartab_pop(varstack);

  /* `exit` inside `eval` longjmped past source_popfd; recover here.
     Each popped frame may also own an fd (source_popfd() = source_pop()
     + fd_pop()) still linked into the current fdstack level's fd list;
     pop that too or it's left dangling. */
  while(source && source != e->source) {
    struct fd* f = source->fd;
    source_pop();

    if(f)
      fd_pop(f);
  }

  // sh->exitcode = e->exitcode;
  // sh->eval = e->parent;
  eval = e->parent;

  return ret;
}
