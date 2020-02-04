#include "../shell.h"
#include "../str.h"
#include "../windoze.h"
#if WINDOWS_NATIVE
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <limits.h>

#if WINDOWS_NATIVE
#define PATHSEP_C '\\'
#else
#define PATHSEP_C '/'
#endif

static int
getsep(const char* path) {
  while(*path) {
    if(*path == '/' || *path == '\\')
      return *path;
    ++path;
  }
  return '\0';
}

/* get current working directory into a stralloc
 */
void
shell_getcwd(stralloc* sa) {
  char sep;
  stralloc_zero(sa);
  /* reserve some space */
  stralloc_ready(sa, PATH_MAX);
  /* repeat until we have reserved enough space */
  getcwd(sa->s, sa->a);
  sa->len = str_len(sa->s);
  sep = getsep(sa->s);
}
