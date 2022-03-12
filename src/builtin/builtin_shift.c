#include "../fdtable.h"
#include "../../lib/scan.h"
#include "../sh.h"

/* ----------------------------------------------------------------------- */
int
builtin_shift(int argc, char* argv[]) {
  unsigned int n = 1;

  if(argv[1]) {
    if(scan_uint(argv[1], &n) == 0) {
      sh_msg(argv[0]);
      buffer_putm_internal(fd_err->w, ": ", argv[1], ": invalid argument", NULL);
      buffer_putnlflush(fd_err->w);
      return 1;
    }
  }

if(n > sh->arg.c)
  n = sh->arg.c;


  while(sh->arg.c && n--) {
    sh->arg.s++;
    sh->arg.v++;
    sh->arg.c--;
  }

  return 0;
}
