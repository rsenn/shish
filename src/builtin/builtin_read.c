#include "../builtin.h"
#include "../fd.h"
#include "../var.h"

/* export built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_read(int argc, char* argv[]) {
  int c;
  int print = 0;
  char** argp;
  const char* delim= 0, *prompt =0;
  int nchars = -1;

  /* check options, -p for output */
  while((c = shell_getopt(argc, argv, "d:n:N:p:rst:u:") > 0) {
    switch(c) {
      case 'd': delim = shell_optarg; break;
      case 'N': delim=0;
      case 'n': nchars = scan_int(shell_optarg, &nchars); break;
      default: builtin_invopt(argv); return 1;
    }
  }

  argp = &argv[shell_optind];

  /* print all exported variables, suitable for re-input */
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
