#include "../fdtable.h"
#include "../sh.h"
#include "../builtin.h"
#include "../exec.h"
#include "../parse.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"

/* Helper function to check if a name is a shell keyword */
static inline int
is_keyword(const char* str) {
  int i;

  for(i = TI_NOT; i <= TI_END; i++)
    if(str_equal(parse_tokens[i].name, str))
      return 1;
  return 0;
}

/* command built-in
 *
 * ----------------------------------------------------------------------- */
const char help_command[] =
    "    Run a command, bypassing any shell function of the same name.\n"
    "\n"
    "    -p              use a default PATH guaranteed to find the standard\n"
    "                    utilities\n"
    "    -v              print the resolved path/description instead of\n"
    "                    running the command\n"
    "    -V              like -v, but more verbose\n"
    "    command         command to run, bypassing any shell function of\n"
    "                    the same name\n"
    "    arg             arguments passed to command\n";

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

  /* Check if it's a keyword first (for -v and -V) */
  if((print_desc || print_verbose) && is_keyword(name)) {
    if(print_verbose)
      buffer_putm_internal(fd_out->w, name, " is a shell keyword", NULL);
    else
      buffer_puts(fd_out->w, name);
    buffer_putnlflush(fd_out->w);
    return 0;
  }

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
    if(EXIT_NOEXEC > (ret = exec_command(&cmd, argc - shell_optind, &argv[shell_optind], 0)))
      return ret;
  }

  /* at this point the exec stuff failed */
  sh_error_errno(argv[shell_optind]);
  return exec_error();
}
