#include "../builtin.h"
#include "../exec.h"
#include "../../lib/shell.h"

/* type built-in
 * ----------------------------------------------------------------------- */
int
builtin_type(int argc, char* argv[]) {
  int c, all_locations = 0, suppress_functions = 0, force_path = 0,
         print_path = 0, type_name = 0;
  char* name;

  /* check options */
  while((c = shell_getopt(argc, argv, "afPpt")) > 0) {
    switch(c) {
      case 'a': all_locations = 1; break;
      case 'f': suppress_functions = 1; break;
      case 'P': force_path = 1; break;
      case 'p': print_path = 1; break;
      case 't': type_name = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* no arguments? return now! */
  if(!(name = argv[shell_optind]))
    return 0;

  exec_type(name, suppress_functions ? H_FUNCTION : 0, force_path, type_name);

  return 0;
}
