#include "dirlist_internal.h"

/* dirlist_skip returns 1 if name is not listed under flags.
 *
 *   ".git"  NOHIDDEN: skipped      "."  ".."  DOTS: kept, else skipped
 *   NOHIDDEN wins over DOTS         anything else: kept
 * ----------------------------------------------------------------------- */
int
dirlist_skip(const char* name, unsigned flags) {
  if(name[0] != '.')
    return 0;
  if(flags & DIRLIST_NOHIDDEN)
    return 1;
  if(!name[1] || (name[1] == '.' && !name[2]))
    return !(flags & DIRLIST_DOTS);
  return 0;
}
