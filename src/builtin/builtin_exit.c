#include "../fd.h"
#include "../sh.h"
#include "../../lib/scan.h"

/* exit built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_exit(int argc, char* argv[]) {
  int status = 0;

  if(argc > 1)
    scan_int(argv[1], &status);
  else
    status = sh->exitcode;

  status &= 0xff;

  sh_exit(status);

  /* should never return! */
  return 0;
}
