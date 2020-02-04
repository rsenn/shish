#include "term.h"
#include <stdlib.h>

extern stralloc term_cmdline;

/* get command line
 *
 * (return value has to be free'd)
 * ----------------------------------------------------------------------- */
char*
term_getline(void) {
  char* ret;

  stralloc_nul(&term_cmdline);

  if(!term_cmdline.s[0])
    return NULL;

  ret = term_cmdline.s;
  term_cmdline.s = NULL;

  return ret;
}
