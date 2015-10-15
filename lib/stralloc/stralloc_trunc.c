#include "stralloc.h"
#include "shell.h"

/* truncates to n+1 and nul-terminates (but '\0' is not included in len)  */
int stralloc_trunc(stralloc *sa, unsigned long int n) {
  if((sa->s = shell_realloc(sa->s, n + 1))) {
    sa->s[n] = '\0';
    sa->len = n;
    return 1;
  }
  return 0;
}

