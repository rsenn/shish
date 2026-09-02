#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../vartab.h"
#include "../source.h"
#include "../sh.h"

/* readonly built-in
 *
 * ----------------------------------------------------------------------- */
const char help_readonly[] = "    Mark variables so they cannot be changed or unset.\n"
                             "\n"
                             "    -p              print every readonly variable as 'NAME=VALUE'\n"
                             "    name            variable to make readonly\n"
                             "    name=value      assign then make the variable readonly\n";

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
    if((*argp)[str_chr(*argp, '=')]) {
      struct var* v;

      /* Check if variable exists and is readonly. POSIX requires special
         builtins to exit the shell on error in non-interactive mode
         -- sh_interactive (sh.h), the whole session's own
         interactive-ness, not source->mode's per-buffer
         SOURCE_IACTIVE, which would wrongly fire inside any
         `.`-sourced file even in an interactive session. */
      if((v = var_search(*argp, NULL)) != NULL && (v->flags & V_READONLY)) {
        builtin_errmsg(argv, *argp, "readonly variable");

        if(!sh_interactive)
          sh_exit(1);

        return 1;
      }

      var_copys(*argp, V_READONLY);
    }

    /* and now apply the readonly flag change */
    var_chflg(*argp, V_READONLY, 1);
  }

  return 0;
}
