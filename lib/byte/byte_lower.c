#include "../byte.h"

void
byte_lower(void* out, size_t len) {
  unsigned char* s = out;

  for(size_t i = 0; i < len; i++)
    if(s[i] >= 'A' && s[i] <= 'Z')
      s[i] += 'a' - 'A';
}
