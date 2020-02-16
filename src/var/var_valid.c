#include "../parse.h"

/* check if it is a valid variable name
 * ----------------------------------------------------------------------- */
int
var_valid(const char* v) {
  const char* s;

  if(parse_isdigit(*v))
    return 0;

  for(s = v; *s; s++) {
    if(!parse_isname(*s))
      break;
  }
  if(s > v && (!*s || *s == '='))
    return 1;
  return 0;
}
