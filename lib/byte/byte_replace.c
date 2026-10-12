
#include "../byte.h"

void
byte_replace(char* x, size_t n, char before, char after) {
  for(size_t i = 0; i < n; i++)
    if(x[i] == before)
      x[i] = after;
}
