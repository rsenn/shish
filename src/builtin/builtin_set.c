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
  int debug = 0, exit_on_err = 0, no_clobber = 0, error_on_unset = 0;

  /* check options */
  while((c = shell_getopt(argc, argv, "xeCu")) > 0) {
    switch(c) {
      case 'x': debug = 1; break;
      case 'e': exit_on_err = 1; break;
      case 'C': no_clobber = 1; break;
      case 'u': error_on_unset = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(debug)
    sh->flags |= SH_DEBUG;
  if(exit_on_err)
    sh->flags |= SH_ERREXIT;
  if(no_clobber)
    sh->flags |= SH_NOCLOBBER;
  if(error_on_unset)
    sh->flags |= SH_UNSET;

  if(argv[shell_optind])
    sh_setargs(&argv[shell_optind], 1);
  else if(!(debug || exit_on_err))
    vartab_print(V_DEFAULT);

  return 0;
}
