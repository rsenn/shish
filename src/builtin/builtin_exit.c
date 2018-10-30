#include "scan.h"
#include "sh.h"

/* exit built-in 
 * 
 * ----------------------------------------------------------------------- */
int builtin_exit(int argc, char **argv) {
  int status = 0;

  if(argc > 1)
    scan_uint(argv[1], (unsigned int*)&status);
  else
    status = sh->exitcode;

  sh_exit(status);

  /* should never return! */
  return 0;
}

