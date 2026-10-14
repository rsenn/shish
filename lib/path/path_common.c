#include "../path_internal.h"
#include "../byte.h"

/* path_common returns the length of the common whole-component prefix of a and b, in a, and sets *blen
 * to its length in b; trailing separators are excluded, repeated separators compare equal.
 *
 *   "/usr/local/bin","//usr//local//share" -> 10, *blen 12    "a/b","c/b" -> 0, 0    "/a","a" -> 0, 0
 */
size_t
path_common(const char* a, size_t an, const char* b, size_t bn, size_t* blen) {
  size_t ra = path_rootlen(a, an), rb = path_rootlen(b, bn), ia, ib, ea, eb, ca, cb;

  if(ra != rb || byte_diff(a, ra, b)) {
    if(blen)
      *blen = 0;
    return 0;
  }

  ia = ra + path_seplen(a + ra, an - ra);
  ib = rb + path_seplen(b + rb, bn - rb);
  ea = ra;
  eb = rb;

  for(;;) {
    ca = path_comlen(a + ia, an - ia);
    cb = path_comlen(b + ib, bn - ib);

    if(ca == 0 || ca != cb || byte_diff(a + ia, ca, b + ib))
      break;

    ea = ia + ca;
    eb = ib + cb;
    ia = ea + path_seplen(a + ea, an - ea);
    ib = eb + path_seplen(b + eb, bn - eb);
  }

  if(blen)
    *blen = eb;

  return ea;
}
