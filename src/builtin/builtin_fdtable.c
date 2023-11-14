#include "../builtin.h"
#include "../fd.h"
#include "../../lib/buffer.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/windoze.h"
#include "../../lib/scan.h"
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
  int i, c, fd = 1;

  while((c = shell_getopt(argc, argv, "u:")) > 0) {
    switch(c) {

      case 'u': scan_int(shell_optarg, &fd); break;
      default: builtin_invopt(argv); return 1;
    }
  }

  buffer_puts(fdtable[fd]->w, " vfd  rfd  wfd  lev  file\n");

  fdtable_foreach(i) fd_print(fdtable[i], fdtable[fd]->w);

  buffer_flush(fdtable[fd]->w);

  return 0;
}
