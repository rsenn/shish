#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../vartab.h"

/* export built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_export(int argc, char** argv) {
  int c;
  int clear = 0;
  int print = 0;
  char** argp;

  /* check options, -n for unexport, -p for output */
  while((c = shell_getopt(argc, argv, "np")) > 0) {
    switch(c) {
      case 'n': clear = 1; break;
      case 'p': print = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  argp = &argv[shell_optind];

  /* print all exported variables, suitable for re-input */
  if(*argp == NULL || print) {
    vartab_print(V_EXPORT);
    return 0;
  }
  /* export each argument */
  for(; *argp; argp++) {
    if(!var_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid identifier");
      continue;
    }

    /* if there is a = we assign the variable first */
    if((*argp)[str_chr(*argp, '=')])
      var_copys(*argp, V_EXPORT);

    /* and now apply the export flag change */
    var_chflg(*argp, V_EXPORT, !clear);
  }

  return 0;
}
