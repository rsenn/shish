#include "../builtin.h"
#include "../sh.h"
#include "../var.h"
#include "../../lib/shell.h"

static struct optstate state;

/* getopts built-in
 * ----------------------------------------------------------------------- */
int
builtin_getopts(int argc, char* argv[]) {
  const char *optstring, *name;

  if(argc < 1) {
    builtin_errmsg(argv, "optstring argument required", NULL);
    return 1;
  }
  if(argc < 2) {
    builtin_errmsg(argv, "name argument required", NULL);
    return 1;
  }

  optstring = argv[1];
  name = argv[2];

  if(!var_valid(name)) {
    builtin_errmsg(argv, name, "not a valid identifier");
    return 2;
  }

  {
    int ac;
    char** av;
    int c;
    char ch;
    int ret = 0;

    if(argc > 2) {
      ac = argc - 2;
      av = argv + 2;
    } else {
      ac = sh->arg.c;
      av = sh->arg.v;
    }

    c = shell_getopt_r(&state, ac, av, optstring);

    switch(c) {
      case '?': ret = 1; break;
      default:
        ch = c;
        var_setv(name, &ch, 1, V_LOCAL);
        var_setvint("OPTIND", state.ind,  V_LOCAL);

        if(state.arg) var_setv("OPTARG", state.arg, str_len(state.arg), V_LOCAL);
        else var_unset("OPTARG");

        break;
    }

    return ret;
  }
}
