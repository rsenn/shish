#include "../fd.h"
#include "../fdtable.h"

/* wish 'e' become the new expected fd before calling open()/dup()
 * ----------------------------------------------------------------------- */
int
fdtable_wish(int e, int flags) {
  /* if the wished position is above the bottom we can maybe get it
     by lazy resolving. */
  if(e > fd_expected)
    return fdtable_lazy(e, flags);

  /* if the wished position is below the bottom it is occupied by
     another (fd). try to make a gap there */
  if(e < fd_expected) {
    if(flags & FDTABLE_CLOSE)
      return e;

    return fdtable_gap(e, flags);
  }

  /* e == fd_expected, so the wish is already satisfied */
  return FDTABLE_DONE;
}
