#include "../builtin.h"
#include "../sh.h"
#include "../var.h"
#include "../exec.h"
#include "../../lib/str.h"
#include "../../lib/shell.h"

struct optstate builtin_getopts_state = {0};

#define optarg builtin_getopts_state.arg
#define optind builtin_getopts_state.ind
#define optofs builtin_getopts_state.ofs
#define optopt builtin_getopts_state.opt

/* getopts built-in
 * ----------------------------------------------------------------------- */
int
builtin_getopts(int argc, char* argv[]) {
  char *optstring, *name;

  if(argc < 2) {
    builtin_errmsg(argv, "optstring argument required", NULL);
    return 1;
  }

  if(argc < 3) {
    builtin_errmsg(argv, "name argument required", NULL);
    return 1;
  }

  optstring = argv[1];
  name = argv[2];

  if(!var_valid(name)) {
    builtin_errmsg(argv, name, "not a valid identifier");
    return EXIT_ERROR;
  }

  {
    int ac;
    char** av;
    int c;
    int ret = 0;

    if(argc > 3) {
      ac = argc - 2;
      av = argv + 2;
    } else {
      ac = sh->arg.c;
      av = sh->arg.v;
    }

    c = shell_getopt_r(&builtin_getopts_state, ac + 1, av - 1, optstring);
    var_unset("OPTARG");

    switch(c) {
      case '?': ret = 2; break;
      case -1: ret = 1; break;

      default:
        if(optarg)
          var_setv("OPTARG", optarg, str_len(optarg), V_LOCAL);

        break;
    }

    if(c != -1) {
      char ch = optopt;
      var_setv(name, &ch, 1, V_LOCAL);
    } else {
      var_unset(name);
    }

    var_setvint("OPTIND", optind, V_LOCAL);

    return ret;
  }
}
