#include <string.h>
#include <errno.h>
#include "shell.h"
#include "str.h"
#include "fd.h"
#include "sh.h"
#include "builtin.h"

/* builtin helpers
 * ----------------------------------------------------------------------- */
int builtin_errmsgn(char **argv, const char *s, unsigned int n, char *msg) {
  sh_msg(argv[0]);
  buffer_putm(fd_err->w, ": ", NULL);
  buffer_put(fd_err->w, s, n);
  if(msg)
    buffer_putm(fd_err->w, ": ", msg, NULL);
  buffer_putnlflush(fd_err->w);
  return 1;
}

int builtin_errmsg(char **argv, char *s, char *msg) {
  return builtin_errmsgn(argv, s, str_len(s), msg);
}

int builtin_error(char **argv, char *s) {
  return builtin_errmsg(argv, s, strerror(errno));
}
  
int builtin_invopt(char **argv) {
  char optchars[2];
  optchars[0] = '-';
  optchars[1] = shell_optopt;
  return builtin_errmsgn(argv, optchars, 2, "invalid option");
}


