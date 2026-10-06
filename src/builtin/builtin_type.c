#include "builtin_config.h"
#include "../builtin.h"
#include "../exec.h"
#include "../../lib/shell.h"

#if BUILTIN_TYPE

/* type built-in
 * ----------------------------------------------------------------------- */
const char help_type[] = "    Show how a name would be interpreted if run as a command.\n"
                         "\n"
                         "    -a              print every match: alias, keyword, function, builtin, each PATH file\n"
                         "    -f              suppress function matches\n"
                         "    -P              force a PATH search, even for a builtin/function\n"
                         "    -p              print the path only, if name resolves to a file\n"
                         "    -t              print just the type (alias/function/builtin/file)\n"
                         "    name            name to look up\n";

int
builtin_type(int argc, char* argv[]) {
  int c, all_locations = 0, suppress_functions = 0, force_path = 0, print_path = 0, type_name = 0;
  int status = 0;

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

  /* one line per operand; status 1 if any is not found */
  for(; argv[shell_optind]; shell_optind++) {
    char* name = argv[shell_optind];

    if(!exec_type(name, suppress_functions ? H_FUNCTION : 0, force_path, type_name, print_path, all_locations))
      continue;
    if(!type_name && !print_path)
      builtin_errmsg(argv, name, "not found");
    status = 1;
  }

  return status;
}

#endif /* BUILTIN_TYPE */
