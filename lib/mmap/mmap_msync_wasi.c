#ifdef __wasi__
#include <sys/mman.h>

/* wasi-libc's emulated mmap has no msync(); mappings are read-only copies,
 * so there is nothing to flush.
 * ----------------------------------------------------------------------- */
int
msync(void* addr, size_t len, int flags) {
  (void)addr, (void)len, (void)flags;
  return 0;
}
#endif
