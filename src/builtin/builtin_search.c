#include "builtin.h"

/* find a builtin which has flags set or any builtin if flag is 0
 * ----------------------------------------------------------------------- */
struct builtin_cmd *builtin_search(const char *name, int flags) {
  unsigned int bi, ni;

  for(bi = 0; builtin_table[bi].name; bi++) {
    for(ni = 0; name[ni] == builtin_table[bi].name[ni]; ni++) {
      if(name[ni] == '\0') {
        if(builtin_table[bi].name[ni] == '\0' &&
           (builtin_table[bi].flags & flags) == flags)
          return &builtin_table[bi];
        
        break;
      }
    }
  }

  return NULL;
}
