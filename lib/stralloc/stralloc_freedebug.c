#include "shell.h"
#include "stralloc.h"

#ifdef DEBUG

void stralloc_freedebug(const char *file, unsigned int line, stralloc *sa) {
  if (sa->s) debug_free(file, line, sa->s);
  sa->s = 0;
}
#endif /* defined(DEBUG) */