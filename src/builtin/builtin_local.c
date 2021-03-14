#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"

/* local built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_local(int argc, char* argv[]) {
  int c;
  int print = 0;
  char** argp;

  /* check options, -p for output */
  /*  while((c = shell_getopt(argc, argv, "p")) > 0) {
      switch(c) {
        case 'p': print = 1; break;
        default: builtin_invopt(argv); return 1;
      }
    }*/

  argp = &argv[1 /*shell_optind*/];

  /* print all local variables, suitable for re-input */
  /*  if(*argp == NULL || print) {
      vartab_print(V_READONLY);
      return 0;
    }*/

  /* set each argument */
  for(; *argp; argp++) {
    if(!var_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid identifier");
      continue;
    }

    /* if there is a = we assign the variable first */
    if((*argp)[str_chr(*argp, '=')])
      var_copys(*argp, V_READONLY);
    else
      /* and now apply the local flag change */
      var_chflg(*argp, V_READONLY, 1);
  }

  return 0;
}
