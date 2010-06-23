#include "source.h"
#include "fd.h"

/* ----------------------------------------------------------------------- */
int source_get(unsigned char *c)
{
  int ret;
  ret = buffer_getc(source->b, c);

  if(ret >= 0)
  {
    if(*c == '\n')
      source_newline();
  }
  
  return ret;
}

