#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/uint32.h"
#include "../parse.h"
#include "../var.h"
#include "../tree.h"
#include "../exec.h"
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
const char help_unset[] = "    Unset variables or functions.\n"
                          "\n"
                          "    -f              treat each name as a function\n"
                          "    -v              treat each name as a variable\n"
                          "    name            variable or function to unset (function first,\n"
                          "                    if neither -f nor -v is given and both exist)\n";

int
builtin_unset(int argc, char* argv[]) {
  int c, fun = 0, var = 0;
  char** argp;

  /* check options, -n for unexport, -p for output */
  while((c = shell_getopt(argc, argv, "fv")) > 0) {
    switch(c) {
      case 'f': fun = 1; break;
      case 'v': var = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

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
        uint32 h;
        struct exechash* e;

        *nptr = fn->next;
        fn->next = 0;
        tree_free(fn);

        /* Invalidate exec_hash cache for this function name.
           Without this, type/eval will still find the stale cached
           entry pointing at the freed function body. */
        e = exec_lookup(*argp, &h);
        if(e)
          e->mask = -1;

        continue;
      }
    }

    if(!fun) {
      struct var* v;

      /* check if variable exists and is readonly */
      if((v = var_search(*argp, NULL)) != NULL && (v->flags & V_READONLY)) {
        builtin_errmsg(argv, *argp, "readonly variable");
        continue;
      }

      if(var_unset(*argp))
        continue;
    }

    // builtin_errmsg(argv, *argp, fun ? "no such function" : var ? "no such variable" : "no such
    // variable/function");
  }

  return 0;
}
