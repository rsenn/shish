#include "../path_internal.h"

/* lexical canonical form of p[0..n) into out; returns its length.
 * out may equal p. Nothing is written unless cap >= n + 2, otherwise n + 1 (the bound) is returned.
 *
 *   "a//b/./c/../d" -> "a/b/d"   "/.." -> "/"   "../.." -> "../.."   "" -> "."   "a/.." -> "."
 * ----------------------------------------------------------------------- */
size_t
path_canon(char* out, size_t cap, const char* p, size_t n) {
  size_t root, base, w, r, c;

  if(cap < n + 2)
    return n + 1;

  root = path_rootlen(p, n);

  if(out != p)
    byte_copy(out, root, p);

  w = base = root;
  r = root + path_seplen(p + root, n - root);

  while(r < n) {
    c = path_comlen(p + r, n - r);

    if(c == 1 && p[r] == '.') {
    } else if(c == 2 && p[r] == '.' && p[r + 1] == '.') {
      if(w > base) {
        while(w > base && !path_issep(out[w - 1]))
          --w;

        if(w > base)
          --w;
      } else if(!root) {
        if(w)
          out[w++] = PATHSEP_C;

        out[w++] = '.';
        out[w++] = '.';
        base = w;
      }
    } else {
      if(w > root)
        out[w++] = PATHSEP_C;

      byte_copy(out + w, c, p + r);
      w += c;
    }

    r += c;
    r += path_seplen(p + r, n - r);
  }

  if(w == 0)
    out[w++] = '.';

  out[w] = '\0';
  return w;
}
