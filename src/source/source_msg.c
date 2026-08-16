#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"

/* source msg
 * ----------------------------------------------------------------------- */
void
source_msg(const struct location* pos) {
  /* fdtable[STDSRC_FILENO] can be NULL here: fdtable_exec() pops
     every pushed source right before execve(). If execve() then
     fails, the error path runs with no source left, so fall back to
     sh_name. */
  const char* name = fdtable[STDSRC_FILENO] ? fdtable[STDSRC_FILENO]->name : NULL;

  if(!(source->mode & SOURCE_IACTIVE)) {
    buffer_puts(fd_err->w, name ? name : sh_name);
    buffer_putc(fd_err->w, ':');
    buffer_putulong(fd_err->w, pos->line);
    buffer_putc(fd_err->w, ':');
    buffer_putulong(fd_err->w, pos->column);
    buffer_puts(fd_err->w, ": ");
  }
}
