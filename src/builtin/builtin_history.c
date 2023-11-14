#include "../builtin.h"
#include "../history.h"
#include "../../lib/shell.h"

/* manage command history
 * ----------------------------------------------------------------------- */
int
builtin_history(int argc, char* argv[]) {
  int c, clear = 0;

  /* check options, -c for clear */
  while((c = shell_getopt(argc, argv, "c")) > 0) {
    switch(c) {
      case 'c': clear = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(clear)
    history_clear();
  else
    history_print();

  return 0;
}
