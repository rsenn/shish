#include "byte.h"
#include "tree.h"
#include "parse.h"

void parse_init(struct parser *p, int flags) {
  byte_zero(p, sizeof(struct parser));
  p->flags = flags;
}
