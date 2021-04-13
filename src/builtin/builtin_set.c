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
  struct shopt opts = sh->opts;

  /* check options */
  while((c = shell_getopt(argc, argv, "+efhmuxBCH")) > 0) {
    int on = shell_optprefix == '-';
    switch(c) {
      case 'e': opts.errexit = on; break;
      case 'f': opts.noglob = on; break;
      case 'h': opts.hashall = on; break;
      case 'm': opts.monitor = on; break;
      case 'u': opts.unset = on; break;
      case 'x': opts.xtrace = on; break;
      case 'B': opts.braceexpand = on; break;
      case 'C': opts.noclobber = on; break;
      case 'H': opts.histexpand = on; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  sh->opts = opts;

  if(argv[shell_optind])
    sh_setargs(&argv[shell_optind], 1);
  else if(byte_count(&opts, sizeof(opts), 0) == sizeof(opts))
    vartab_print(V_DEFAULT);

  return 0;
}
