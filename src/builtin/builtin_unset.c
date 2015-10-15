#include "shell.h"
#include "var.h"
#include "builtin.h"

/* unset built-in 
 * 
 * ----------------------------------------------------------------------- */
int builtin_unset(int argc, char **argv) {
  int c;
  int fun = 0;
  int var = 0;
  char **argp;

  /* check options, -n for unexport, -p for output */
  while((c = shell_getopt(argc, argv, "fv")) > 0) {
    switch(c) {
      case 'f': fun = 1; break;
      case 'v': var = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }
  
  // TODO:
  (void)fun;
  (void)var;
  
  argp = &argv[shell_optind];

  /* unset each argument */
  for(; *argp; argp++) {
    if(!var_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid identifier");
      continue;
    }
    
    var_unset(*argp);
  }

  return 0;
}

