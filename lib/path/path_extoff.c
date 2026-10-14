#include "internal.h"

/* path_extoff returns the offset of the extension dot in the basename of p[0..n), or n if it has none.
 * A leading dot is not an extension; the last dot wins.
 *
 *   "a/b.tar.gz" -> offset of ".gz"   ".bashrc" -> n   "a.d/b" -> n   "a." -> offset of "."
 */
size_t
path_extoff(const char* p, size_t n) {
  size_t len, i;
  const char* b = path_basename(p, n, &len);

  if(len == 2 && b[0] == '.' && b[1] == '.')
    return n;

  for(i = len; i > 1; --i)
    if(b[i - 1] == '.')
      return (size_t)(b - p) + i - 1;

  return n;
}
