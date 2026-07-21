#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"

/* source msg
 * ----------------------------------------------------------------------- */
void
source_msg(const struct location* pos) {
  /* fdtable[STDSRC_FILENO] can be NULL here: fdtable_exec() pops every
     pushed source right before execve(), on the assumption that a
     successful exec makes them moot. If execve() then fails, the
     error-reporting path (exec_program() -> sh_error_errno() ->
     sh_msgn() -> here) runs with no source left on the stack at all --
     fall back to sh_name the same way the SOURCE_IACTIVE-off branch
     below already does when a source has no name of its own. */
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
