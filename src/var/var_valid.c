#include "../parse.h"

/* check if it is a valid variable name
 * ----------------------------------------------------------------------- */
int
var_valid(const char* v) {
  size_t i;

  if(parse_isdigit(*v))
    return 0;

  for(i = 0; v[i]; i++) {
    if(v[i] == '=')
      break;

    if(!parse_isname(v[i], i))
      return 0;
  }
  return 1;
}
