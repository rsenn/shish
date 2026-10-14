#include "internal.h"

/* lexical canonical form of p[0..n) into out; returns its length.
 * out may equal p. Nothing is written unless cap >= n + 2, otherwise n + 1 (the bound) is returned.
 * Off Windows a leading run of separators is one (like realpath -m -s); "//host" stays on Windows.
 *
 *   "a//b/./c/../d" -> "a/b/d"   "/.." -> "/"   "../.." -> "../.."   "" -> "."   "a/.." -> "."
 *   "//a" -> "/a"
 * ----------------------------------------------------------------------- */
size_t
path_canon(char* out, size_t cap, const char* p, size_t n) {
  size_t root, ro, base, w, r, c;

  if(cap < n + 2)
    return n + 1;

  ro = root = path_rootlen(p, n);

#if !WINDOWS
  if(ro > 1)
    ro = 1;
#endif

  if(out != p)
    byte_copy(out, ro, p);

  w = base = ro;
  r = root + SEPLEN(p + root, n - root);

  while(r < n) {
    c = COMLEN(p + r, n - r);

    if(c == 1 && p[r] == '.') {
    } else if(c == 2 && p[r] == '.' && p[r + 1] == '.') {
      if(w > base) {
        while(w > base && !SEP(out[w - 1]))
          --w;

        if(w > base)
          --w;
      } else if(!ro) {
        if(w)
          out[w++] = PATHSEP_C;

        out[w++] = '.';
        out[w++] = '.';
        base = w;
      }
    } else {
      if(w > ro)
        out[w++] = PATHSEP_C;

      byte_copy(out + w, c, p + r);
      w += c;
    }

    r += c;
    r += SEPLEN(p + r, n - r);
  }

  if(w == 0)
    out[w++] = '.';

  out[w] = '\0';
  return w;
}
