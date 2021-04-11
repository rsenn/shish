#include "../buffer.h"
#include "../path_internal.h"
#include "../scan.h"
#include "../windoze.h"
/* get home directory into a char buf not smaller than PATH_MAX + 1 chars
 */
char*
path_gethome(int uid) {
  buffer b;
  long id;
  size_t n;
  static char home[PATH_MAX + 1];
  /* try to read /etc/passwd */
  if(buffer_mmapread(&b, "/etc/passwd"))
    return NULL;
  for(;;) {
    /* skip the next two colon-separators to get the uid */
    if(buffer_skip_until(&b, ":", 1) <= 0)
      break;
    if(buffer_skip_until(&b, ":", 1) <= 0)
      break;
    /* get the uid or the home */
    if((n = buffer_get_until(&b, home, PATH_MAX + 1, ":", 1)) <= 0)
      break;
    /* remove the trailing delimiter */
    if(n && home[n - 1] == ':')
      n--;
    home[n] = '\0';
    /* if we have an uid match the token is the home */
    if(id == uid) {
      buffer_close(&b);
      return home;
    }
    /* parse the uid */
    if(!scan_ulong(home, (unsigned long*)&id))
      id = -1;
    /* on a mismatch -> get the next line */
    if(id != uid) {
      if(buffer_skip_until(&b, "\n\r", 2) <= 0)
        break;
    }
  }
  buffer_close(&b);
  return NULL;
}
