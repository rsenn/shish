#include "../../lib/byte.h"
#include "parse.h"
#include "tree.h"

void
parse_init(struct parser* p, int flags) {
  byte_zero(p, sizeof(struct parser));
  p->flags = flags;
}
