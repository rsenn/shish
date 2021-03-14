#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../vartab.h"

static int
alias_valid(const char* v) {
  size_t i;
  for(i = 0; v[i] && v[i] != '='; i++) 
   if(!parse_isname(v[i], i) && !(i > 0 && v[i] == '-'))
      return 0;
  return 1;
}


/* alias built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_alias(int argc, char* argv[]) {
  int c;
   int print = 0;
  char** argp;

   while((c = shell_getopt(argc, argv, "p")) > 0) {
    switch(c) {
       case 'p': print = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  argp = &argv[shell_optind];

  /* print all aliases, suitable for re-input */
  if(*argp == NULL || print) {
     return 0;
  }

  /* add each alias */
  for(; *argp; argp++) {
    if(!alias_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid alias name");
      continue;
    }

  }

  return 0;
}
