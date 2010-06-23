#include <unistd.h>
#include "stralloc.h"
#include "shell.h"
#include "str.h"

char *shell_gethostname(stralloc *sa)
{
  unsigned long n = 0;
  
  do
  {
    n += 64;
    stralloc_ready(sa, n);
  }
  while(gethostname(sa->s, sa->a));
    
  if(sa->s)
  {
    sa->s[sa->a - 1] = '\0';
    sa->len = str_len(sa->s);
  }
  
  return sa->s;
}

  
