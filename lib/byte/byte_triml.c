#include "../byte.h"

const char*
byte_triml(const char* x, size_t* len, const char* charset, unsigned int charsetlen) {
  size_t i, n = *len;
  for(i = 0; i < n; ++i) {
    if(byte_chr(charset, charsetlen, x[i]) == charsetlen)
      break;
  }
  *len = n - i;
  return x + i;
}
