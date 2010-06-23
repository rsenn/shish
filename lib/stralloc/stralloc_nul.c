#include <stralloc.h>

int stralloc_nul(stralloc *sa)
{
  if(stralloc_ready(sa,sa->len+1)) 
  {
    sa->s[sa->len]='\0';
    return 1;
  }
  return 0;
}
