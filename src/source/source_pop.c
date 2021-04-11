#include "../source.h"
#include <assert.h>

/* cleanup source stuff
 * ----------------------------------------------------------------------- */
void
source_pop(void) {
  assert(source);

  source = source->parent;
}
