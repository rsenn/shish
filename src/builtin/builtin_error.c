#include "../builtin.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include <errno.h>
#include <string.h>

/* builtin helpers
 * ----------------------------------------------------------------------- */
int
builtin_errmsgn_nonl(char* argv[], const char* s, unsigned int n, char* msg) {
  sh_msg(argv[0]);
  buffer_puts(fd_err->w, ": ");
  buffer_put(fd_err->w, s, n);

  if(msg) {
    buffer_puts(fd_err->w, ": ");
    buffer_puts(fd_err->w, msg);
  }

  return 1;
}

int
builtin_errmsgn(char* argv[], const char* s, unsigned int n, char* msg) {
  builtin_errmsgn_nonl(argv, s, n, msg);
  buffer_putnlflush(fd_err->w);
  return 1;
}

int
builtin_errmsg_nonl(char* argv[], char* s, char* msg) {
  return builtin_errmsgn_nonl(argv, s, str_len(s), msg);
}

int
builtin_errmsg(char* argv[], char* s, char* msg) {
  return builtin_errmsgn(argv, s, str_len(s), msg);
}

int
builtin_error(char* argv[], char* s) {
  return builtin_errmsg(argv, s, strerror(errno));
}

/* the synopsis line and the help text of a builtin, as "help NAME" prints them */
void
builtin_usage(const struct builtin_cmd* b, buffer* out) {
  buffer_puts(out, b->name);
  buffer_putspace(out);
  buffer_puts(out, b->args);
  buffer_putnlflush(out);
  buffer_puts(out, b->help ? b->help : "    No further help available.\n");
  buffer_flush(out);
}

/* reports what shell_getopt() just returned; the value to return from the builtin:
 *   --help            help on stdout, 0
 *   anything else     message and "usage: NAME ARGS" on stderr, 2
 * ----------------------------------------------------------------------- */
int
builtin_invopt(char* argv[]) {
  const struct builtin_cmd* b;
  char optchars[2];
  const char* what = optchars;
  size_t n = 2;
  char* msg = "invalid option";

  for(b = builtin_table; b->name; b++)
    if(!str_diff(b->name, argv[0]))
      break;

  if(shell_opterr == SHELL_OPTERR_HELP) {
    if(b->name)
      builtin_usage(b, fd_out->w);

    return 0;
  }

  if(shell_optlname) {
    what = shell_optlname;
    n = str_chr(what, '=');
  } else {
    optchars[0] = '-';
    optchars[1] = shell_optopt;
  }

  if(shell_opterr == SHELL_OPTERR_MISSING)
    msg = "option requires an argument";
  else if(shell_opterr == SHELL_OPTERR_NOARG)
    msg = "option doesn't take an argument";

  builtin_errmsgn(argv, what, n, msg);

  if(b->name) {
    buffer_puts(fd_err->w, "usage: ");
    buffer_puts(fd_err->w, b->name);
    buffer_putspace(fd_err->w);
    buffer_puts(fd_err->w, b->args);
    buffer_putnlflush(fd_err->w);
  }

  return 2;
}
