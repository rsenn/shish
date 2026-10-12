#include "../str.h"
#include "../stralloc.h"

/* stralloc_copy copies the string stored in sa2 into sa. It is the same
 * as stralloc_copyb(&sa, sa2.s, sa2.len). sa2 must already be allocated. */
int
stralloc_copy(stralloc* sa, const stralloc* sa2) {
  return stralloc_copyb(sa, sa2->s, sa2->len);
}
