#include "../stralloc.h"
#include "../byte.h"

void
stralloc_replacec(stralloc* sa, char before, char after) {
  byte_replace(sa->s, sa->len, before, after);
}
