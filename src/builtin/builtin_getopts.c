#include "../builtin.h"
#include "../sh.h"
#include "../var.h"
#include "../exec.h"
#include "../../lib/str.h"
#include "../../lib/scan.h"
#include "../../lib/shell.h"

struct optstate builtin_getopts_state = {0};

#define optarg builtin_getopts_state.arg
#define optind builtin_getopts_state.ind
#define optofs builtin_getopts_state.ofs
#define optopt builtin_getopts_state.opt

/* getopts built-in
 * ----------------------------------------------------------------------- */
const char help_getopts[] =
    "    Parse positional (or given) arguments option by option.\n"
    "\n"
    "    optstring       letters getopts recognizes; a trailing ':' on a\n"
    "                    letter means it takes an argument (in $OPTARG)\n"
    "    name            variable set to the option letter found, or '?'\n"
    "    arg             arguments to parse instead of $1.. ($OPTIND advances)\n";

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
    /* a leading ':' in optstring is getopts' own convention (distinct
       from shell_getopt_r's ':'-means-optional-argument one) asking
       for silent error reporting: no diagnostics on stderr, and
       OPTARG/name carry the offending option character instead */
    int silent = optstring[0] == ':';

    if(argc > 3) {
      ac = argc - 3;
      av = argv + 3;
    } else {
      ac = sh->arg.c;
      av = sh->arg.v;
    }

    /* $OPTIND, not our persistent parser state, is the authoritative
       next-argument index -- POSIX lets a script reset or reposition
       it. Re-sync whenever it no longer matches what we last wrote,
       resetting to 0 (not 1) so shell_getopt_r() runs its own
       first-call init instead of us half-repeating it here. */
    {
      const char* v;
      size_t offset;
      unsigned int cur_optind;

      if((v = var_get("OPTIND", &offset)) && scan_uint(&v[offset], &cur_optind) &&
         cur_optind != (unsigned int)optind) {
        optind = cur_optind <= 1 ? 0 : (int)cur_optind;
        optofs = 0;
        optopt = 0;
        optarg = 0;
      }
    }

    {
      /* shell_getopt_r()'s ind==0 sentinel always becomes 1 during its
         own first-call init, before it even looks at an option --
         predict that so the "did optind advance" check below isn't
         fooled by the init bump on the very first getopts call. */
      int ind_before = optind == 0 ? 1 : optind;

      c = shell_getopt_r(&builtin_getopts_state, ac + 1, av - 1, optstring);

      /* shell_getopt_r() defers advancing past a plain boolean flag's
         argv element in case more flags follow in the same "-vf"
         cluster. If nothing's left in that element either, finish
         the advance ourselves so $OPTIND already names the next
         element by the time this call returns, matching bash/dash. */
      if(c > 0 && optind == ind_before) {
        char* cur = av[optind - 1];

        if(cur && cur[optofs + 1] == '\0') {
          optind++;
          optofs = 0;
        }
      }
    }

    var_unset("OPTARG");

    switch(c) {
      case -1: ret = 1; break;

      case '?':
        if(silent) {
          char ch = optopt;
          var_setv("OPTARG", &ch, 1, V_LOCAL);
        } else {
          char optchars[2] = {'-', (char)optopt};
          builtin_errmsgn(argv, optchars, 2, "illegal option");
        }
        break;

      case ':':
        if(silent) {
          char ch = optopt;
          var_setv("OPTARG", &ch, 1, V_LOCAL);
        } else {
          char optchars[2] = {'-', (char)optopt};
          builtin_errmsgn(argv, optchars, 2, "option requires an argument");
          /* non-silent mode reports a missing argument the same way
             as an unknown option: name='?', no OPTARG */
          c = '?';
        }
        break;

      default:
        if(optarg)
          var_setv("OPTARG", optarg, str_len(optarg), V_LOCAL);

        break;
    }

    {
      char ch = c == -1 ? '?' : c;
      var_setv(name, &ch, 1, V_LOCAL);
    }

    var_setvint("OPTIND", optind, V_LOCAL);

    return ret;
  }
}
