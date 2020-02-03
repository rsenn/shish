#include "debug.h"

#ifdef DEBUG_OUTPUT
#include "builtin.h"
#include "fdstack.h"
#include "fdtable.h"
#include "shell.h"
#include "vartab.h"

/* ----------------------------------------------------------------------- */
int
builtin_dump(int argc, char** argv) {
  int c;

  while((c = shell_getopt(argc, argv, "vltsfm")) > 0) {
    switch(c) {
      case 'v': vartab_dump(&vartab_root); break;
      case 'l': vartab_dump(NULL); break;
      case 't': fdtable_dump(); break;
      case 's': fdstack_dump(); break;
      case 'f': fd_dumplist(); break;
      case 'm': debug_memory(); break;
      default: builtin_invopt(argv); return 1;
    }
  }

  return 0;
}
#endif /* DEBUG_OUTPUT */
