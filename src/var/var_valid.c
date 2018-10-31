#include "parse.h"

/* check if it is a valid variable name 
 * ----------------------------------------------------------------------- */
int var_valid(const char *v) {
  register const char *s = v;
  
  if(parse_isdigit(*v)) return 0;
  
  for(;;) {
    if(!parse_isname(*s++)) return 0;
    if(!*s || *s == '=') return 1;
  }
}
