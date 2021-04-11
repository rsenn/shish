#include "../sh.h"
#include "../../lib/path.h"
#include "../var.h"

/* read home directory from /etc/passwd without the whole getpw*() bloat :)
 * ----------------------------------------------------------------------- */
const char*
sh_gethome(void) {
  const char* v;
  size_t offset;

  /* HOME variable is set, copy it from there */
  if((v = var_get("HOME", &offset)))
    return &v[offset];

  return path_gethome(sh_uid);
}
