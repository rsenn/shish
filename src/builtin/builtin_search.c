#include "../builtin.h"
#include "../../lib/str.h"

/* find a builtin which has flags set or any builtin if flag is 0
 * ----------------------------------------------------------------------- */
struct builtin_cmd*
builtin_search(const char* name, builtin_flag flags) {
  size_t bi;
  struct builtin_cmd* ret = 0;

  for(bi = 0; builtin_table[bi].name && builtin_table[bi].fn; bi++) {
    if((builtin_table[bi].flags == flags) && !str_diff(builtin_table[bi].name, name)) {
      ret = &builtin_table[bi];
      break;
    }
  }

  return ret;
}
