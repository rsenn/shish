#include "../builtin.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../tree.h"
#include "../../lib/shell.h"
#include "../vartab.h"

extern union node* functions;

/* set arguments of flags
 * ----------------------------------------------------------------------- */
const char help_set[] =
    "    Set shell options and/or positional parameters.\n"
    "\n"
    "    -e              exit if a simple command fails\n"
    "    -f              disable pathname expansion (globbing)\n"
    "    -h              remember command locations as they're looked up\n"
    "    -m              enable job control (monitor mode)\n"
    "    -u              treat unset variables as an error on expansion\n"
    "    -x              print each command before running it\n"
    "    -B              enable brace expansion\n"
    "    -C              don't let '>' clobber an existing file\n"
    "    -H              enable history expansion ('!')\n"
    "    +option         turn the option off instead of on\n"
    "    arg             new positional parameters ($1, $2, ...)\n"
    "\n"
    "    With no options or arguments, print every variable and function.\n";

int
builtin_set(int argc, char* argv[]) {
  int c, got_opt = 0;
  struct shopt opts = sh->opts;
  struct optstate opt = {"+-", 0, 0, 0, 0, 0};

  /* check options */
  while((c = shell_getopt_r(&opt, argc, argv, "+efhmuxBCH")) > 0) {
    int on = opt.prefix == '-';

    got_opt = 1;

    switch(c) {
      case 'e': opts.errexit = on; break;
      case 'f': opts.noglob = on; break;
      case 'h': opts.hashall = on; break;
      case 'm': opts.monitor = on; break;
      case 'u': opts.unset = on; break;
      case 'x': opts.xtrace = on; break;
      case 'B': opts.braceexpand = on; break;
      case 'C': opts.noclobber = on; break;
      case 'H': opts.histexpand = on; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  sh->opts = opts;

  if(argv[opt.ind])
    sh_setargs(&argv[opt.ind], 1);
  else if(!got_opt) {
    union node* n;

    vartab_print(V_DEFAULT);

    for(n = functions; n; n = n->next) {
      tree_print(n, fd_out->w);
      buffer_putnlflush(fd_out->w);
    }
  }

  return 0;
}
