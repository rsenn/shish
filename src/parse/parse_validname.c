#include "tree.h"
#include "parse.h"

/* check whether the word is a valid name according to 3.1.5
 * ----------------------------------------------------------------------- */
int parse_validname(stralloc *sa) {
  unsigned long i;

  /* there must be string data */
  if(!sa->len || sa->s == NULL)
    return 0;

  /* check chars and return on mismatch */
  for(i = 0; i < sa->len; i++)
    if(!parse_isname(sa->s[i]))
      return 0;

  return 1;
}

