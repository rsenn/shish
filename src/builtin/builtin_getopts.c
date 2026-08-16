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
       next-argument index: POSIX lets a script reset it to 1 to
       restart option parsing (over the same or new arguments), and
       real-world scripts also reposition it to arbitrary values to
       skip parsing ahead. Re-sync our state to it whenever it no
       longer matches what we wrote back after the previous call --
       setting it back to 0 (rather than 1) for a restart lets
       shell_getopt_r() run its own first-call initialization
       (prefix/optstring setup) instead of us half-repeating it here. */
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
      /* shell_getopt_r()'s ind==0 sentinel always becomes 1 during
         its own first-call init (see the resync block above, which
         already relies on that same convention), before it even
         looks at an option -- predict that so the "did this call
         actually advance past its argv element" check below isn't
         fooled by the init bump on a getopts call that's the very
         first one ever made */
      int ind_before = optind == 0 ? 1 : optind;

      c = shell_getopt_r(&builtin_getopts_state, ac + 1, av - 1, optstring);

      /* shell_getopt_r() defers advancing past a plain boolean flag's
         argv element (no ':' in optstring) in case more flags follow
         in the same "-vf"-style cluster; optind not moving at all
         during the call above is exactly that case. If nothing is
         left in that element either, finish the advance ourselves
         right now (both are otherwise-untouched updates -- ofs=0,
         ind+1 -- shell_getopt_r would apply lazily on the *next*
         call's "end of argument" check anyway, so doing it here
         changes nothing about how parsing continues) so $OPTIND
         already names the next element to process by the time this
         call returns, matching bash/dash, instead of lagging by one
         call. */
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
