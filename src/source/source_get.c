#include "../fd.h"
#include "../source.h"

/* ----------------------------------------------------------------------- */
int
source_get(char* c) {
  int ret;
  ret = source_peek(c);

  if(ret >= 1) {
    source_skip();
    /*if(*c == '\n')
       source_newline();*/
  }

  return ret;
}
