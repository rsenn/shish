#include "../str.h"

int
str_case_diffn(const char* p1, const char* p2, size_t n) {
  int x;
  int y;
  const char* s;
  const char* t;

  if(n == 0)
    return 0;
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
    if(s - (const char*)p1 >= (ssize_t)n)
      break;

    if(x != y)
      break;

    if(!x)
      break;

    if(!y)
      break;
  }
  return x - y;
}
