#include "str.h"
#include "buffer.h"
#include "shell.h"

/* output error message
 * 
 * always returns 1 so it can be used as exit code for builtins
 * ----------------------------------------------------------------------- */
int shell_error(const char *s)
{
  return shell_errorn(s, str_len(s));
}

