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

  if(print_verbose && !print_desc)
    return exec_type(name, H_FUNCTION, 0, 1);

  /* look up the command and exec if found */
  if((cmd = exec_hash(name, H_FUNCTION)).ptr) {

    if(print_desc) {
      buffer_puts(fd_out->w, cmd.id == H_PROGRAM ? cmd.path : name);
      buffer_putnlflush(fd_out->w);
      return 0;
    }

    /* try to exec */
    if(EXIT_NOEXEC > (ret = exec_command(&cmd, argc - shell_optind, &argv[shell_optind], 0, NULL)))
      return ret;
  }

  /* at this point the exec stuff failed */
  sh_error(argv[shell_optind]);
  return exec_error();
}
