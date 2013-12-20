#include <stdlib.h>
#include "stralloc.h"
#include "shell.h"

void stralloc_free(stralloc *sa) {
  if (sa->s) shell_free(sa->s);
  sa->s=0;
}
