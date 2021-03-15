#include "../builtin.h"
#include "../fd.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../vartab.h"

/* set arguments of flags
 * ----------------------------------------------------------------------- */
int
builtin_set(int argc, char* argv[]) {
  int c;
  union shopt opts = {0};

  /* check options */
  while((c = shell_getopt(argc, argv, "xeCu")) > 0) {
    switch(c) {
      case 'x': opts.debug = 1; break;
      case 'e': opts.exit_on_error = 1; break;
      case 'C': opts.no_clobber = 1; break;
      case 'u': opts.unset = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  sh->opts = opts;

  if(argv[shell_optind])
    sh_setargs(&argv[shell_optind], 1);
  else if(!opts.flags)
    vartab_print(V_DEFAULT);

  return 0;
}
