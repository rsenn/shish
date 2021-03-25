#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../parse.h"
#include "../var.h"
#include "../tree.h"
extern union node* functions;

static inline union node**
find_function(const char* name) {

  union node** nptr = &functions;

  for(nptr = &functions; *nptr; nptr = tree_next(nptr)) {
    struct nfunc* fn = &(*nptr)->nfunc;
    if(!str_diff(fn->name, name))
      return nptr;
  }
  return 0;
}

/* unset built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_unset(int argc, char* argv[]) {
  int c;
  int fun = 0;
  int var = 0;
  char** argp;

  /* check options, -n for unexport, -p for output */
  while((c = shell_getopt(argc, argv, "fv")) > 0) {
    switch(c) {
      case 'f': fun = 1; break;
      case 'v': var = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* TODO:*/
  (void)fun;
  (void)var;

  argp = &argv[shell_optind];

  /* unset each argument */
  for(; *argp; argp++) {
    if(fun && !parse_isfuncname(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid function name");
      continue;
    }

    if(var && !var_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid variable name");
      continue;
    }

    if(!var) {
      union node** nptr;
      if((nptr = find_function(*argp))) {
        union node* fn = *nptr;
        *nptr = fn->next;
        fn->next = 0;
        tree_free(fn);
        continue;
      }
    }
    if(!fun) {
      if(var_unset(*argp))
        continue;
    }

    // builtin_errmsg(argv, *argp, fun ? "no such function" : var ? "no such variable" : "no such variable/function");
  }

  return 0;
}
