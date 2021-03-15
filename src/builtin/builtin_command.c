#include "../fd.h"
#include "../sh.h"
#include "../builtin.h"
#include "../exec.h"
#include "../../lib/shell.h"

/* command built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_command(int argc, char* argv[]) {
  int c, default_path = 0, print_desc = 0, print_verbose = 0;
  struct command cmd;
  char* name;
  int ret = 1;

  /* check options, -l for login dash, -c for null env, -a to set argv[0] */
  while((c = shell_getopt(argc, argv, "pvV")) > 0) {
    switch(c) {
      case 'p': default_path = 1; break;
      case 'v': print_desc = 1; break;
      case 'V': print_verbose = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* no arguments? return now! */
  if(!(name = argv[shell_optind]))
    return 0;

  if(print_desc || print_verbose)
    return exec_type(name, H_FUNCTION, 0, !print_verbose);

  /* look up the command and exec if found */
  if((cmd = exec_hash(name, H_FUNCTION)).ptr) {

    /* try to exec */
    if(126 >
       (ret =
            exec_command(&cmd, argc - shell_optind, &argv[shell_optind], 0, NULL)))
      return ret;
  }

  /* at this point the exec stuff failed */
  sh_error(argv[shell_optind]);
  return exec_error();
}
