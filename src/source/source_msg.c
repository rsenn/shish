#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"

/* source msg
 * ----------------------------------------------------------------------- */
void
source_msg(const struct location* pos) {
  const char* name = fdtable[STDSRC_FILENO]->name;
 
  if(!(source->mode & SOURCE_IACTIVE)) {
    buffer_puts(fd_err->w, name ? name : sh_name);
    buffer_putc(fd_err->w, ':');
    buffer_putulong(fd_err->w, pos->line);
    buffer_putc(fd_err->w, ':');
    buffer_putulong(fd_err->w, pos->column);
    buffer_puts(fd_err->w, ": ");
  }
}
