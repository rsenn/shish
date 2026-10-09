#include "../fd.h"
#include "../source.h"

/* ----------------------------------------------------------------------- */
int
source_get(char* c) {
  int ret;

  if((ret = source_peek(c)) > 0) 
    source_skip();

  return ret;
}
