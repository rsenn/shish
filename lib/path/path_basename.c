/* from dietlibc by felix leitner, adapted to libowfat */
#include "../path_internal.h"
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
path_basename(const char* path) {
  char* x = (char*)path;
  size_t n;
again:
  n = str_rchrs(x, "/\\", 2);
  if(x[n] == '\0')
    return x;
  if(x[n + 1] == 0) {
    if(n == 0)
      return x;
    else {
      (x)[n] = 0;
      goto again;
    }
  }
  return &x[n + 1];
}
