#include "../../lib/str.h"
#include "../fd.h"
#include "../sh.h"
#include "../source.h"

/* output message prefix ("argv[0]: ")
 * ----------------------------------------------------------------------- */
void
sh_msgn(char* s, size_t n) {
  buffer_puts(fd_err->w, sh_name);
  buffer_puts(fd_err->w, ": ");

  if(!(source->mode & SOURCE_IACTIVE))
    source_msg();

  if(s)
    buffer_put(fd_err->w, s, n);
}

void
sh_msg(char* s) {
  return sh_msgn(s, str_len(s));
}
