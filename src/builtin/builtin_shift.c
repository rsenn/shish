#include "../fdtable.h"
#include "../../lib/scan.h"
#include "../sh.h"

/* ----------------------------------------------------------------------- */
const char help_shift[] = "    Shift positional parameters to the left.\n"
                          "\n"
                          "    n               number of positions to shift by (default 1)\n";

int
builtin_shift(int argc, char* argv[]) {
  unsigned int n = 1;

  if(argc > 1) {
    /* Parse the argument */
    if(!scan_uint(argv[1], &n)) {
      /* Invalid argument */
      sh_msg(argv[0]);
      buffer_putm_internal(fd_err->w, ": ", argv[1], ": invalid argument", NULL);
      buffer_putnlflush(fd_err->w);
      return 1;
    }
  }

  /* POSIX: if n > $#, exit > 0 and leave $# unchanged (silently --
     this is not an error, just shift's normal way of reporting that
     it didn't shift) */
  if(n > sh->arg.c)
    return 1;

  /* Perform the shift */
  while(n--) {
    sh->arg.s++;
    sh->arg.v++;
    sh->arg.c--;
  }

  return 0;
}
