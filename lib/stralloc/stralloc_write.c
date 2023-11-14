#include "../buffer.h"
#include "../stralloc.h"
#include "../byte.h"

/* stralloc_catb adds the string buf[0], buf[1], ... buf[len - 1] to the
 * end of the string stored in sa, allocating space if necessary, and
 * returns len. If sa is unallocated, stralloc_catb is the same as
 * stralloc_copyb. If it runs out of memory, stralloc_catb leaves sa
 * alone and returns 0. */
int
stralloc_write(int fd, const char* buf, size_t len, buffer* b) {
  stralloc* sa = (stralloc*)b->cookie;

  if(stralloc_readyplus(sa, len)) {
    byte_copy(sa->s + sa->len, len, buf);
    sa->len += len;
    return len;
  }
  return 0;
}
