#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../vartab.h"

/* export built-in
 *
 * ----------------------------------------------------------------------- */
const char help_export[] =
    "    Mark variables for export to child process environments.\n"
    "\n"
    "    -n              remove the export attribute instead of setting it\n"
    "    -p              print every exported variable as 'export NAME=VALUE'\n"
    "    name            variable to export (defined in the shell if unset)\n"
    "    name=value      assign then export the variable\n";

int
builtin_export(int argc, char* argv[]) {
  int c, clear = 0, print = 0;
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
    if((*argp)[str_chr(*argp, '=')]) {
      struct var* v;

      /* Check if variable exists and is readonly. Reject the assignment
         but don't kill the shell — export readonly errors are not fatal
         even though export is a special builtin (matches bash/dash). */
      if((v = var_search(*argp, NULL)) != NULL && (v->flags & V_READONLY)) {
        builtin_errmsg(argv, *argp, "readonly variable");
        continue;
      }
      
      var_copys(*argp, V_EXPORT);
    }

    /* and now apply the export flag change */
    var_chflg(*argp, V_EXPORT, !clear);
  }

  return 0;
}
