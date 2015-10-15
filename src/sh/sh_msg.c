#include "source.h"
#include "fd.h"
#include "sh.h"

/* output message prefix ("argv[0]: ")
 * ----------------------------------------------------------------------- */
void sh_msg(char *s) {
  buffer_puts(fd_err->w, sh_name);
  buffer_puts(fd_err->w, ": ");

  if(source->mode & SOURCE_IACTIVE)
    source_msg();

  if(s)
    buffer_puts(fd_err->w, s);
}
