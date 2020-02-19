#include "../builtin.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* fdtable built-in - shows open files and stuff
 *
 * note that the virtual file descriptor 's' is the source which the shell
 * parses (which may obviously differ from stdin).
 *
 * an effective filedescriptor of "m" means that the fd is
 * memory-mapped
 *
 * "b" for effective filedescriptor means that the fd has been
 * redirected to an internal write-buffer (for command substitution) or
 * redirected from an internal read-buffer (for the here-docs).
 * ----------------------------------------------------------------------- */
int
builtin_fdtable(int argc, char* argv[]) {
  int i;

  buffer_puts(fd_out->w, " vfd  rfd  wfd  lev  file\n");

  fdtable_foreach(i) fd_print(fdtable[i]);

  buffer_flush(fd_out->w);
  return 0;
}
