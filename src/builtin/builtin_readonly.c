#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../vartab.h"

/* readonly built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_readonly(int argc, char* argv[]) {
  int c, print = 0;
  char** argp;

  /* check options, -p for output */
  while((c = shell_getopt(argc, argv, "p")) > 0) {
    switch(c) {
      case 'p': print = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  argp = &argv[shell_optind];

  /* print all readonly variables, suitable for re-input */
  if(*argp == NULL || print) {
    vartab_print(V_READONLY);
    return 0;
  }

  /* set each argument */
  for(; *argp; argp++) {
    if(!var_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid identifier");
      continue;
    }

    /* if there is a = we assign the variable first */
    if((*argp)[str_chr(*argp, '=')])
      var_copys(*argp, V_READONLY);

    /* and now apply the readonly flag change */
    var_chflg(*argp, V_READONLY, 1);
  }

  return 0;
}
