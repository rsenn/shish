#include "builtin_config.h"

#if BUILTIN_LOCAL

#include "../builtin.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../vartab.h"
#include "../fdtable.h"

/* local built-in
 *
 * ----------------------------------------------------------------------- */
const char help_local[] = "    Declare variables local to the current function.\n"
                          "\n"
                          "    name            declare name local, unset\n"
                          "    name=value      declare name local and assign value\n"
                          "    (no arguments)  print the local variables, suitable for re-input\n";

/* is the vartab part of the current function's scope?
 * (the topmost table, up to and including the nearest function table)
 * ----------------------------------------------------------------------- */
static int
in_function_scope(const struct vartab* tab) {
  const struct vartab* t;

  for(t = varstack; t; t = t->parent) {
    if(t == tab)
      return 1;

    if(t->function)
      break;
  }

  return 0;
}

int
builtin_local(int argc, char* argv[]) {
  char** argp = &argv[1 /*shell_optind*/];

  /* print all local variables, suitable for re-input: "local name="value"" */
  if(*argp == NULL) {
    struct var* var;

    for(var = var_list; var; var = var->gnext) {
      if(var->child == NULL && (var->flags & V_LOCAL) && in_function_scope(var->table)) {
        buffer_puts(fd_out->w, "local ");
        var_print(var, V_DEFAULT);
      }
    }

    return 0;
  }

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
#endif /* BUILTIN_LOCAL */
