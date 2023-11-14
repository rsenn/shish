#include "../byte.h"

size_t
byte_trimr(char* x, size_t n, const char* trimchars, unsigned int trimcharslen) {
  while(n > 0) {
    if(byte_chr(trimchars, trimcharslen, x[n - 1]) == trimcharslen)
      break;
    --n;
  }
  return n;
}
