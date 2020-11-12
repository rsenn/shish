#include "../fd.h"
#include "../source.h"

/* skip current char and get peek next one
 * ----------------------------------------------------------------------- */
int
source_next(char* c) {
  source_skip();

  return c ?  source_peek(c) : 1;
}
