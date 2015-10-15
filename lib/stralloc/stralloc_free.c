#include "shell.h"
#include "stralloc.h"

void stralloc_free(stralloc *sa) {
  if (sa->s) shell_free(sa->s);
  sa->s = 0;
}
