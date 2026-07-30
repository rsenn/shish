#include "../builtin.h"
#include "../exec.h"
#include "../../lib/shell.h"

/* type built-in
 * ----------------------------------------------------------------------- */
const char help_type[] =
    "    Show how a name would be interpreted if run as a command.\n"
    "\n"
    "    -a              print every matching location, not just the first\n"
    "    -f              suppress function matches\n"
    "    -P              force a PATH search, even for a builtin/function\n"
    "    -p              print the path only, if name resolves to a file\n"
    "    -t              print just the type (alias/function/builtin/file)\n"
    "    name            name to look up\n";

int
builtin_type(int argc, char* argv[]) {
  int c, all_locations = 0, suppress_functions = 0, force_path = 0, print_path = 0, type_name = 0;
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
