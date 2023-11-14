#include "../source.h"

/* gets more data from buffer (at least 1 char) and reads first char, but
 * doesn't advance buffer pointer, use source_skip() for that
 * ----------------------------------------------------------------------- */
int
source_peek(char* c) {
  return source_peekn(c, 0);
}

int
source_peekc() {
  char c = 0;

  if(source_peekn(&c, 0) <= 0)
    return -1;
  return (unsigned int)(unsigned char)c;
}
