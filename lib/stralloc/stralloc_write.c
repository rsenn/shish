#include "../buffer.h"
#include "../stralloc.h"
#include "../byte.h"

/* stralloc_catb adds the string buf[0], buf[1], ... buf[len - 1] to the
 * end of the string stored in sa, allocating space if necessary, and
 * returns len. If sa is unallocated, stralloc_catb is the same as
 * stralloc_copyb. If it runs out of memory, stralloc_catb leaves sa
 * alone and returns 0. */
/* the signature below matches buffer_op_proto (../buffer.h) exactly --
 * this function is only ever used as a buffer's write callback
 * (buffer_init(), fd_subst.c), assigned to a buffer_op_proto* field,
 * so unlike an incompatible-signature cast (the previous
 * "int(int,const char*,size_t,buffer*)"), calling through that
 * pointer is fully defined rather than UB (found via
 * -fsanitize=function). */
ssize_t
stralloc_write(int fd, void* vbuf, size_t len, void* arg) {
  const char* buf = vbuf;
  buffer* b = arg;
  stralloc* sa = (stralloc*)b->cookie;

  if(stralloc_readyplus(sa, len)) {
    byte_copy(sa->s + sa->len, len, buf);
    sa->len += len;
    return len;
  }
  return 0;
}
