#include "parse.h"

/* get length of a variable + value without ansi escape sequences */
unsigned long var_vlen(const char *v) {
  const char *s;
  unsigned long n = 0;
  
  for(s = v; *s; s++) {
    if(*s == 0x1b) {
      if(s[1] == '[') {
        s++;
        
        while(parse_isdigit(s[1]) || s[1] == ';')
          s++;
        
        s++;
      }
      
      continue;
    }
    
    n++;
  }
  
  return n;
}
  
