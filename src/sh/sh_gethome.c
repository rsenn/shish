#include <shell.h>
#include "var.h"
#include "sh.h"

/* read home directory from /etc/passwd without the whole getpw*() bloat :)
 * ----------------------------------------------------------------------- */
const char *sh_gethome(void)
{
  const char *v;
  unsigned long offset;

  /* HOME variable is set, copy it from there */
  if((v = var_get("HOME", &offset)))
    return &v[offset];
  
  return shell_gethome(sh_uid);
}

