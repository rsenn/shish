#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"
#include "../../lib/shell.h"

/* output error message
 *
 * does NOT append strerror(errno): errno is whatever the last syscall
 * anywhere left it, not necessarily related to this message. callers
 * reporting a failed syscall want sh_errorn_errno() instead.
 * ----------------------------------------------------------------------- */
int
sh_errorn(const char* s, unsigned int len) {
  sh_msgn(s, len);
  buffer_putnlflush(fd_err->w);
  return 1;
}
