#include "fd.h"
#include "sh.h"
#include "source.h"

/* source msg
 * ----------------------------------------------------------------------- */
void
source_msg(void) {
  if(fdtable[STDSRC_FILENO]->name) {
    buffer_puts(fd_err->w, fdtable[STDSRC_FILENO]->name);
    buffer_puts(fd_err->w, ":");
    buffer_putulong(fd_err->w, source->line);
    buffer_puts(fd_err->w, ": ");
  }
}
