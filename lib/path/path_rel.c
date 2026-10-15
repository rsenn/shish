#include "internal.h"
#include "../byte.h"

/* path_rel writes the lexical relative path from base to p into out and returns its length.
 * Both must be canonical (see path_canon). Nothing is written unless cap > length.
 * Returns (size_t)-1 if one is absolute and the other relative, or their roots differ.
 *
 *   "/a/b/c","/a/d" -> "../b/c"   "/a","/a" -> "."   "a","a/b/c" -> "../.."   "/a","b" -> -1
 */
size_t
path_rel(char* out, size_t cap, const char* p, size_t pn, const char* base, size_t bn) {
  size_t ca, cb, up = 0, i, len;
  const char* rest;

  if(!SAMEROOT(p, path_rootlen(p, pn), base, path_rootlen(base, bn)))
    return (size_t)-1;

  ca = path_common(p, pn, base, bn, &cb);
  rest = p + ca + SEPLEN(p + ca, pn - ca);
  pn -= rest - p;

  while(pn && SEP(rest[pn - 1]))
    --pn;

  for(i = cb + SEPLEN(base + cb, bn - cb); i < bn; i += COMLEN(base + i, bn - i), i += SEPLEN(base + i, bn - i))
    ++up;

  len = up * 2 + (up ? up - 1 : 0) + (up && pn ? 1 : 0) + pn;

  if(len == 0)
    len = 1;

  if(cap > len) {
    char* o = out;

    for(i = 0; i < up; ++i) {
      if(i)
        *o++ = PATHSEP_C;

      *o++ = '.';
      *o++ = '.';
    }

    if(up && pn)
      *o++ = PATHSEP_C;

    byte_copy(o, pn, rest);
    o += pn;

    if(o == out)
      *o++ = '.';

    *o = '\0';
  }

  return len;
}
