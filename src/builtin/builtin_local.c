#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"

/* local built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_local(int argc, char* argv[]) {
  char** argp;

  argp = &argv[1 /*shell_optind*/];

  /* print all local variables, suitable for re-input */
  /*  if(*argp == NULL || print) {
      vartab_print(V_READONLY);
      return 0;
    }*/

  /* set each argument */
  for(; *argp; argp++) {
    size_t namelen, valuelen;

    if(!var_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid identifier");
      continue;
    }
    namelen = str_chr(*argp, '=');
    valuelen = str_len(*argp) - (namelen + 1);
    /* if there is a = we assign the variable first */
    if((*argp)[namelen] == '\0')
      (*argp)[namelen] = '=';

    var_setv(*argp, *argp + namelen + 1, valuelen, V_LOCAL);
  }

  return 0;
}
