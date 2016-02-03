<<<<<<< HEAD
#ifndef WIN32
=======
#ifdef _WIN32
#include <io.h>
#else
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <unistd.h>
#endif
#include "stralloc.h"
#include "shell.h"
#include "str.h"

<<<<<<< HEAD
#ifdef WIN32
=======
#ifdef _WIN32
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <winsock2.h>
#endif

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

  
