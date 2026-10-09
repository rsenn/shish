#include "../path_internal.h"

/* Normalizes path[0..n) in place, lexically; returns the new length (<= n).
 *
 *   "a//b/./c/"  -> "a/b/c/"     repeated separators and "." go, a trailing one stays
 *   "/a/../.."   -> "/"          ".." above the root of an absolute path is dropped
 *   "a/../.."    -> ".."         leading ".." of a relative path is kept
 *   "a/.."       -> "."          never empty for a non-empty input ("" gives 0)
 * ----------------------------------------------------------------------- */
size_t
path_collapse(char* path, size_t n) {
  size_t r = 0, w = 0, floor, s;
  int trail;
  char sep = PATHSEP_C;

  if(n == 0)
    return 0;

  for(s = 0; s < n; ++s)
    if(path_issep(path[s])) {
      sep = path[s];
      break;
    }

  trail = path_issep(path[n - 1]);

  if(path_isdrive(path) && n >= 2)
    w = r = 2;

  if(r < n && path_issep(path[r])) {
    path[w++] = sep;
    ++r;
  }

  floor = w;

  while(r < n) {
    size_t len;

    while(r < n && path_issep(path[r]))
      ++r;

    if(r >= n)
      break;

    for(s = r; r < n && !path_issep(path[r]); ++r) {}
    len = r - s;

    if(len == 1 && path[s] == '.')
      continue;

    if(len == 2 && path[s] == '.' && path[s + 1] == '.') {
      size_t k = w;

      while(k > floor && !path_issep(path[k - 1]))
        --k;

      /* pop the last component unless there is none or it is itself ".." */
      if(w > floor && !(w - k == 2 && path[k] == '.' && path[k + 1] == '.')) {
        w = k > floor ? k - 1 : k;
        continue;
      }

      /* absolute: nothing above the root */
      if(floor > (path_isdrive(path) ? 2u : 0u))
        continue;
    }

    if(w > floor)
      path[w++] = sep;

    /* w <= s: a forward copy is safe for the overlap, byte_copy() is not */
    for(; len; --len)
      path[w++] = path[s++];
  }

  if(w == floor && floor == (path_isdrive(path) ? 2u : 0u)) {
    /* relative and empty */
    path[w++] = '.';
  }

  if(trail && w > floor && !path_issep(path[w - 1]))
    path[w++] = sep;

  return w;
}
