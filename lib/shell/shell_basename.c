/* from dietlibc by felix leitner, adapted to libowfat */

#include "../shell.h"
#include "../str.h"

/*
       path           dirname        basename
       "/usr/lib"     "/usr"         "lib"
       "/usr/"        "/"            "usr"
       "usr"          "."            "usr"
       "/"            "/"            "/"
       "."            "."            "."
       ".."           "."            ".."
*/

char*
shell_basename(char* path) {
  unsigned int n;
again:
  n = str_rchr(path, '/');
  if(path[n] == '\0')
    return path;
  if(path[n + 1] == 0) {
    if(n == 0)
      return path;
    else {
      path[n] = 0;
      goto again;
    }
  }
  return &path[n + 1];
}
