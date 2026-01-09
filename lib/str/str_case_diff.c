#include "../str.h"

#if LINK_STATIC
int
str_case_diff(const char* p1, const char* p2) {
  int x;
  int y;
  const char* s;
  const char* t;
  s = p1;
  t = p2;

  for(;;) {
    x = *s++;

    if(x >= 'A' && x <= 'Z') { /* upper case */
      x += 'a' - 'A';
    }
    y = *t++;

    if(y >= 'A' && y <= 'Z') { /* upper case */
      y += 'a' - 'A';
    }

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
