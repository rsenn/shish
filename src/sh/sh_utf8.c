#include "../var.h"
#include "../sh.h"
#include "../../lib/utf8.h"

/* whether the shell's locale variables ask for UTF-8 text handling; read on
 * every call (three hash lookups), so an assignment takes effect at once.
 * Builtins ask once when they start and keep the answer.
 * ----------------------------------------------------------------------- */
int
sh_utf8(void) {
  return u8locale(var_value("LC_ALL", NULL), var_value("LC_CTYPE", NULL), var_value("LANG", NULL));
}
