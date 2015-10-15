#include "scan.h"
#include "fd.h"
#include "sh.h"

/* ----------------------------------------------------------------------- */
int builtin_shift(int argc, char **argv)
{
  unsigned int n = 1;
  
  if(argv[1])
  {
    scan_uint(argv[1], &n);

    if(n == 0)
    {
      sh_error(argv[0]);
      buffer_putm(fd_err->w, ": ", argv[1], ": invalid argument");
      buffer_putnlflush(fd_err->w);
      return 1;
    }
  }
  
  while(sh->arg.c && n--) {
    sh->arg.s++;
    sh->arg.a++;
    sh->arg.c--;
  }

  return 0;
}


