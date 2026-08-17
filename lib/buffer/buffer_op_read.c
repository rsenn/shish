#include "../windoze.h"
#include "../buffer.h"

#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* wraps read(2) (3 args) to match buffer_op_proto (4 args) exactly --
 * unlike a raw (buffer_op_proto*)&read cast, calling through this
 * pointer is well-defined C rather than UB, and WebAssembly's
 * call_indirect (which enforces an exact signature match) accepts it. */
ssize_t
buffer_op_read(int fd, void* buf, size_t len, void* arg) {
  (void)arg;
  return read(fd, buf, len);
}
