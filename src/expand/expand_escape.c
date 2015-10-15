#include "str.h"
#include "stralloc.h"

/* escape characters subject to glob() expansion
 * ----------------------------------------------------------------------- */
void expand_escape(stralloc *sa, const char *b, unsigned int n) {
  while(n--) {
    if(str_chr("\\*?[", *b) < 4)
      stralloc_catc(sa, '\\');

    stralloc_catc(sa, *b++);
  }
}

