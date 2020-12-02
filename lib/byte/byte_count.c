#include "../byte.h"

size_t
byte_count(const void* s, size_t n, char c) {
  const unsigned char* t;
  unsigned int count;
  unsigned char ch;
  t = (unsigned char*)s;
  count = 0;
  ch = (unsigned char)c;
  for(;;) {
    if(!n)
      break;
    if(*t == ch) {
      ++count;
    }
    ++t;
    --n;

    if(!n)
      break;
    if(*t == ch) {
      ++count;
    }
    ++t;
    --n;

    if(!n)
      break;
    if(*t == ch) {
      ++count;
    }
    ++t;
    --n;

    if(!n)
      break;
    if(*t == ch) {
      ++count;
    }
    ++t;
    --n;
  }
  return count;
}
