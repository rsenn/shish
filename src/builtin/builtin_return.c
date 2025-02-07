#include "../fdtable.h"
#include "../../lib/scan.h"
#include "../eval.h"
#include "../sh.h"

/* continue/break a loop
 * ----------------------------------------------------------------------- */
int
builtin_return(int argc, char* argv[]) {
  unsigned int ret = 0;

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
