#include "../bre.h"

const char*
bre_match_star(
    struct bre_ctx* ctx, const char* atom, size_t alen, const char* rest, const char* s) {
  size_t k, maxrun = 0;
  while(s[maxrun] && bre_atom_matches(atom, alen, (unsigned char)s[maxrun]))
    maxrun++;
  for(k = maxrun + 1; k-- > 0;) {
    const char* r;
    if((r = bre_match_rec(ctx, rest, s + k)))
      return r;
  }
  return (const char*)0;
}