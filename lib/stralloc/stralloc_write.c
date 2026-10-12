#include "../buffer.h"
#include "../stralloc.h"
#include "../byte.h"

/* the signature below matches buffer_op_proto (../buffer.h) exactly --
 * this function is only ever used as a buffer's write callback
 * (buffer_init()), assigned to a buffer_op_proto* field,
 * so unlike an incompatible-signature cast e.g. that of stralloc_catb(),
 * calling through that pointer is fully defined rather than UB
 * (found via -fsanitize=function). */
ssize_t
stralloc_write(int fd, void* buf, size_t len, void* arg) {
  buffer* b = arg;
  stralloc* sa = (stralloc*)b->cookie;

  if(stralloc_readyplus(sa, len)) {
    byte_copy(sa->s + sa->len, len, buf);
    sa->len += len;
    return len;
  }

  return 0;
}
