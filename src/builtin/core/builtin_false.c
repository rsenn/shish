#include "builtin_config.h"

#if BUILTIN_FALSE

/* return exit status 1
 * ----------------------------------------------------------------------- */
const char help_false[] = "    Do nothing, unsuccessfully (always exits 1).\n";

int
builtin_false(int argc, char* argv[]) {
  return 1;
}
#endif /* BUILTIN_FALSE */
