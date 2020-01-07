#include "history.h"
#include "source.h"
#include "term.h"
#include <assert.h>

/* cleanup source stuff
 * ----------------------------------------------------------------------- */
void
source_pop(void) {
  assert(source);

  if(source->mode & SOURCE_IACTIVE) {
    term_restore(source->b);

    history_save();
    history_clear();
  }

  source = source->parent;
}
