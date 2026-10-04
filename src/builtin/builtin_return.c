#include "../fdtable.h"
#include "../../lib/scan.h"
#include "../eval.h"
#include "../sh.h"
#include "builtin_config.h"

/* return built-in
 * ----------------------------------------------------------------------- */
const char help_return[] =
    "    Return from a shell function or sourced script.\n"
    "\n"
    "    n               exit status to return (default: the last command's)\n";

#include "../trap.h"

int
builtin_return(int argc, char* argv[]) {
  unsigned int ret = sh->exitcode & 0xff;

  /* inside a trap body a bare "return" yields the status from before the trap */
  if(!argv[1] && trap_return_status() >= 0)
    ret = trap_return_status() & 0xff;

  if(argv[1]) {
    if(scan_uint(argv[1], &ret) == 0) {
      sh_error(argv[0]);
      buffer_putm_internal(fd_err->w, ": ", argv[1], ": invalid argument", 0);
      buffer_putnlflush(fd_err->w);
      return 1;
    }
  }

  eval_return(ret);
  return 0;
}
