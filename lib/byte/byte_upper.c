#include "../byte.h"

void
byte_upper(void* out, size_t len) {
  unsigned char* s = out;
  size_t i;

  for(i = 0; i < len; i++)
    if(s[i] >= 'a' && s[i] <= 'z')
      s[i] -= 'a' - 'A';
}
