#include "../../lib/str.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"

/* output message prefix ("argv[0]: ")
 * ----------------------------------------------------------------------- */
void
sh_msgn(const char* s, size_t n) {

  if(source) {
    if((source->mode & SOURCE_IACTIVE)) {
      buffer_puts(fd_err->w, sh_name);
      buffer_puts(fd_err->w, ": ");
    } else {
      source_msg(&source->position);
    }
  }

  if(s)
    buffer_put(fd_err->w, s, n);
}

void
sh_msg(const char* s) {
  sh_msgn(s, str_len(s));
}
