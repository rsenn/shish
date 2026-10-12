#include "../str.h"

#if LINK_STATIC || defined(__dietlibc__)

/* str_case_diff is like str_diff, but it lowercases uppercase characters */
int
str_case_diff(const char* p1, const char* p2) {
  int x, y;
  const char *s = p1, *t = p2;

  for(;;) {
    x = *s++;

    if(x >= 'A' && x <= 'Z') /* upper case */
      x += 'a' - 'A';

    y = *t++;

    if(y >= 'A' && y <= 'Z') /* upper case */
      y += 'a' - 'A';

    if(x != y)
      break;

    if(!x)
      break;

    if(!y)
      break;
  }

  return x - y;
}
#endif
