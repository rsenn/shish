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
 * The string is not modified. len may be NULL when path has no trailing separator.
 * ----------------------------------------------------------------------- */
const char*
path_basename(const char* path, size_t* len) {
  size_t start, end = str_len(path);

  if(end > 0 && path_issep(path[0])) {
    /* all separators -> the first one */
    for(start = 0; start < end && path_issep(path[start]); ++start) {}
    if(start == end) {
      if(len)
        *len = 1;
      return path;
    }
  }

  while(end > 0 && path_issep(path[end - 1]))
    --end;

  for(start = end; start > 0 && !path_issep(path[start - 1]); --start) {}

  if(len)
    *len = end - start;
  return path + start;
}
