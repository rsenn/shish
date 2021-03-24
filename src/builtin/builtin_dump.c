#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../builtin.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../fd.h"
#include "../../lib/shell.h"
#include "../../lib/scan.h"
#include "../vartab.h"

enum { VARTAB_ROOT, VARTAB_LOCAL, FDTABLE, FDSTACK, FDLIST, MEMORY };

/* ----------------------------------------------------------------------- */
int
builtin_dump(int argc, char* argv[]) {
  int c, fd = -1, what = 0, num_args;
  char** argp;
  buffer* out;

  while((c = shell_getopt(argc, argv, "vltsfmu:")) > 0) {
    switch(c) {
      case 'v': what = VARTAB_ROOT; break;
      case 'l': what = VARTAB_LOCAL; break;
      case 't': what = FDTABLE; break;
      case 's': what = FDSTACK; break;
      case 'f': what = FDLIST; break;
      case 'm': what = MEMORY; break;
      case 'u': scan_int(shell_optarg, &fd); break;
      default: builtin_invopt(argv); return 1;
    }
  }
  argp = &argv[shell_optind];
  num_args = argc - shell_optind;
  out = fd == -1 ? buffer_2 : fdtable[fd]->w;

  switch(what) {
    case VARTAB_ROOT: vartab_dump(varstack, num_args, argp); break;
    case VARTAB_LOCAL: vartab_dump(NULL, num_args, argp); break;
    case FDTABLE: fdtable_dump(out); break;
    case FDSTACK: fdstack_dump(out); break;
    case FDLIST: fd_dumplist(out); break;
    case MEMORY:
#ifdef DEBUG_ALLOC
      debug_memory();
#endif
      break;
  }
  return 0;
}
#endif /* DEBUG_OUTPUT */
