#include "../path_internal.h"
#include "../byte.h"

/* Computes the matching prefix offsets for base and target paths.
 * Returns a SizePair containing the consumed lengths (bi, ti). */
SizePair
path_commonlenb(const char* base, size_t blen, const char* target, size_t tlen) {
  const char *b = base, *t = target;
  size_t bn, tn;

  if((bn = path_seplen(b, blen)) && (tn = path_seplen(t, tlen))) {
    b += bn, blen -= bn;
    t += tn, tlen -= tn;
  }

  for(;;) {
    if(!(bn = path_comlen(b, blen)) || !(tn = path_comlen(t, tlen)))
      break;

    if(bn != tn || byte_diff(b, bn, t))
      break;

    b += bn, blen -= bn;
    t += tn, tlen -= tn;

    if(!(bn = path_seplen(b, blen)) || !(tn = path_seplen(t, tlen)))
      break;

    b += bn, blen -= bn;
    t += tn, tlen -= tn;
  }

  b += path_seplen(b, blen);
  t += path_seplen(t, tlen);

  return (SizePair){b - base, t - target};
}