#include "../fd.h"
#include "../sh.h"
#include "../source.h"

/* source msg
 * ----------------------------------------------------------------------- */
void
source_msg(const struct position* pos) {
  const char* name = fdtable[STDSRC_FILENO]->name;
  buffer_puts(fd_err->w, name ? name : sh_name);
  buffer_puts(fd_err->w, ":");
  buffer_putulong(fd_err->w, pos->line);
  buffer_puts(fd_err->w, ": ");
}
