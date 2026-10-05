#include "builtin_config.h"

#if BUILTIN_TRUE

/* return exit status 0
 * ----------------------------------------------------------------------- */
const char help_true[] = "    Do nothing, successfully (always exits 0).\n";

int
builtin_true(int argc, char* argv[]) {
  return 0;
}
#endif /* BUILTIN_TRUE */
