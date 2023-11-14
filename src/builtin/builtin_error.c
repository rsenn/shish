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

int
builtin_invopt(char* argv[]) {
  char optchars[2];
  optchars[0] = '-';
  optchars[1] = shell_optopt;
  return builtin_errmsgn(argv, optchars, 2, "invalid option");
}
