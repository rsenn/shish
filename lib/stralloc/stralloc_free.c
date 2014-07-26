#include "shell.h"
#include "stralloc.h"

#ifdef DEBUG
void stralloc_freedebug(const char *file, unsigned int line, stralloc *sa)
#else
void stralloc_free(stralloc *sa)
#endif /* DEBUG */
{
#ifdef DEBUG
  if (sa->s) debug_free(file, line, sa->s);
#else
  if (sa->s) shell_free(sa->s);
#endif /* DEBUG */
  sa->s=0;
}
