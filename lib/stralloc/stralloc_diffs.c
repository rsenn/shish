#include "../stralloc.h"
#undef stralloc_diffs
#include "../byte.h"
#include "../str.h"

/* stralloc_diffs returns negative, 0, or positive, depending on whether
 * a is lexicographically smaller than, equal to, or greater than the
 * string b[0], b[1], ..., b[n] == '\0'. */
int
stralloc_diffs(const stralloc* a, const char* b) {
  int j;

  for(size_t i = 0;; ++i) {
    if(i == a->len)
      return !b[i] ? 0 : -1;

    if(!b[i])
      return 1;

    if((j = ((unsigned char)(a->s[i]) - (unsigned char)(b[i]))))
      break;
  }

  return j;
}
