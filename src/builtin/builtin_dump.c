#include "../debug.h"
#include "../builtin.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../fd.h"
#include "../tree.h"
#include "../../lib/shell.h"
#include "../../lib/scan.h"
#include "../vartab.h"

enum { VARTAB_ROOT, VARTAB_LOCAL, FDTABLE, FDSTACK, FDLIST, MEMORY, FUNCTIONS };
extern union node* functions;

/* ----------------------------------------------------------------------- */
int
builtin_dump(int argc, char* argv[]) {
  int c, fd = -1, what = 0, num_args;
  char** argp;
  buffer* out;

  while((c = shell_getopt(argc, argv, "Fvltsfmu:")) > 0) {
    switch(c) {
      case 'v': what = VARTAB_ROOT; break;
      case 'l': what = VARTAB_LOCAL; break;
      case 't': what = FDTABLE; break;
      case 's': what = FDSTACK; break;
      case 'f': what = FDLIST; break;
      case 'm': what = MEMORY; break;
      case 'F': what = FUNCTIONS; break;
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
#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDTABLE)
    case FDTABLE: fdtable_dump(out); break;
#endif
#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDSTACK)
    case FDSTACK: fdstack_dump(out); break;
#endif
#if defined(DEBUG_OUTPUT) && defined(DEBUG_FD)
    case FDLIST: fd_dumplist(out); break;
#endif
    case MEMORY:
#ifdef DEBUG_ALLOC
      debug_memory();
#endif
      break;
    case FUNCTIONS: {
      union node* n;
      for(n = functions; n; n = n->next) {
        struct nfunc* fn = &n->nfunc;
        tree_print(n, out);
        buffer_putnlflush(out);
      }
      break;
    }
  }
  return 0;
}
