#include "../history.h"
#include "../source.h"
#include "../term.h"
#include <assert.h>

/* cleanup source stuff
 * ----------------------------------------------------------------------- */
void
source_pop(void) {
  assert(source);

#ifndef SHFORMAT
  if(source->mode & SOURCE_IACTIVE) {
    term_restore(source->b->fd, &term_tcattr);

    history_save();
    history_clear();
  }
#endif

  source = source->parent;
}
