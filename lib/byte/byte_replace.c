#include "../byte.h"

void
byte_replace(char* x, size_t n, char before, char after) {
  size_t i;

  for(i = 0; i < n; i++)
    if(x[i] == before)
      x[i] = after;
}
