#include "sed_internal.h"
#include "../../lib/alloc.h"

void
sed_free(struct sed* prog) {
  size_t i;

  if(!prog)
    return;

  for(i = 0; i < prog->ncmd; i++) {
    struct sed_cmd* c = &prog->cmds[i];

    sed_addr_free(&c->a1);
    sed_addr_free(&c->a2);

    switch(c->letter) {
    case 's': sed_subst_free(&c->u.s); break;
    case 'a': case 'i': case 'c': alloc_free(c->u.text.s); break;
    case 'r': alloc_free(c->u.rfile); break;
    default: break;
    }
  }

  alloc_free(prog->cmds);

  for(i = 0; i < prog->nwfiles; i++)
    alloc_free(prog->wfiles[i].name);

  alloc_free(prog->wfiles);
  alloc_free(prog);
}

int
sed_autoprint_off(const struct sed* prog) {
  return prog->autoprint_off;
}
