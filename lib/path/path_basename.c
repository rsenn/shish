/* from dietlibc by felix leitner, adapted to libowfat */
#include "../path_internal.h"
#include "../str.h"

/* last component of path, POSIX basename(1) rules; *len gets its length
 *
 *   path           basename
 *   "/usr/lib"     "lib"
 *   "/usr/"        "usr"      trailing separators are not part of it
 *   "//"           "/"        only separators: one of them
 *   ""             ""
 *
 * The string is not modified.
 * len may be NULL when path has no trailing separator.
 * ----------------------------------------------------------------------- */
const char*
path_basename(const char* path, size_t* len) {
  const char *x, *y = path + str_len(path);

  if(path < y && path_issep(*path)) {
    /* all separators -> the first one */
    for(x = path; x < y; x++)
      if(!path_issep(*x))
        break;

    if(x == y) {
      if(len)
        *len = 1;

      return path;
    }
  }

  /* Trim trailing separators */
  for(; y > x; y--)
    if(!path_issep(*(y - 1)))
      break;

  /* Scan backwards to find the start of the basename component */
  for(x = y; x > path; x--)
    if(path_issep(*(x - 1)))
      break;

  if(len)
    *len = y - x;

  return x;
}