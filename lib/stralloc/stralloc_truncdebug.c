#include "../alloc.h"
#include "../shell.h"
#include "../stralloc.h"

#ifdef DEBUG_ALLOC

/* truncates to n+1 and nul-terminates (but '\0' is not included in len)  */
int
stralloc_truncdebug(const char* file, unsigned int line, stralloc* sa, unsigned long int n) {
  if((sa->s = alloc_redebug(file, line, sa->s, n + 1))) {
    sa->s[n] = '\0';
    sa->len = n;
    return 1;
  }
  return 0;
}
#endif /* defined(DEBUG_ALLOC) */
