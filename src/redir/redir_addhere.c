#include "../redir.h"
#include "../tree.h"
#include <stdlib.h>

struct nredir* redir_list = NULL;

/* add a here-doc to the pending list, re-deriving the tail each call
 * rather than caching it -- redir_source() may consume entries between
 * two add calls (e.g. `cat <<A <<B`), and the list is always short.
 * ----------------------------------------------------------------------- */
void
redir_addhere(struct nredir* nredir) {
  struct nredir** rptr = &redir_list;

  while(*rptr)
    rptr = (struct nredir**)&(*rptr)->data;

  *rptr = nredir;
  nredir->data = NULL;
}
