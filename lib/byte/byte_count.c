#include "../byte.h"

size_t
byte_count(const void* s, size_t n, char c) {
  const unsigned char* t = (const unsigned char*)s;
  size_t count = 0;
  unsigned char ch = c;
  for(;n;++t,--n)
    if(*t == ch)
      ++count;
  return count;
}
