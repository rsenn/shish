#include "../sh.h"
#include "../../lib/str.h"

/* like sh_error(), but for reporting a syscall that just failed:
 * appends ": <strerror(errno)>" to the message.
 * ----------------------------------------------------------------------- */
int
sh_error_errno(const char* s) {
  return sh_errorn_errno(s, str_len(s));
}
