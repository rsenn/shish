#include "shell.h"
#include "str.h"
#include "sh.h"

/* output error message
 * 
 * always returns 1 so it can be used as exit code for builtins
 * ----------------------------------------------------------------------- */
int sh_error(const char *s) {
  sh_msg(NULL);
  return sh_errorn(s, str_len(s));
}

