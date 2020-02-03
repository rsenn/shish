#include "eval.h"
#include "fd.h"
#include "scan.h"
#include "sh.h"

/* continue/break a loop
 * ----------------------------------------------------------------------- */
int
builtin_break(int argc, char** argv) {
  unsigned int n = 1;

  if(argv[1]) {
    scan_uint(argv[1], &n);

    if(n == 0) {
      sh_error(argv[0]);
      buffer_putm_internal(fd_err->w, ": ", argv[1], ": invalid argument", 0);
      buffer_putnlflush(fd_err->w);
      return 1;
    }
  }

  eval_jump(n, (*argv[0] == 'c'));
  return 0;
}
