#include "internal.h"

/* Joins path parts a[0..an) and b[0..bn) into out, inserting a separator if needed, and returns the required length. */
size_t
path_cat(char* out, size_t cap, const char* a, size_t an, const char* b, size_t bn) {
  size_t need, sep = 0;

  if(bn == 0)
    need = an;
  else if(an == 0 || ISABS(b, bn))
    need = bn, an = 0;
  else {
    sep = !SEP(a[an - 1]);
    need = an + sep + bn;
  }

  if(cap > need) {
    if(an == 0)
      byte_copy(out, bn, b);
    else {
      if(out != a)
        byte_copy(out, an, a);

      if(bn) {
        if(sep)
          out[an] = PATHSEP_C;

        byte_copy(out + an + sep, bn, b);
      }
    }

    out[need] = '\0';
  }

  return need;
}
