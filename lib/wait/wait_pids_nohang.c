#include "../wait.h"

/* drains wait_nohang() (wait for *any* child, non-blocking) until it
 * reports a pid that's in our list, discarding any other children
 * reaped along the way. wait_nohang() already has a WINDOWS_NATIVE
 * implementation, so this needs none of its own -- the old
 * WINDOWS_NATIVE branch that used to live in this file duplicated
 * wait_nohang()'s OpenProcess()/WaitForMultipleObjects(0) logic
 * instead of just calling it (and leaked its HANDLE array on every
 * call besides).
 * ----------------------------------------------------------------------- */
int
wait_pids_nohang(int const* pids, unsigned int len, int* wstat) {
  int r;

  for(;;) {
    int w;

    if((r = wait_nohang(&w)) > 0) {
      unsigned int i = 0;

      for(; i < len; i++)
        if(r == pids[i])
          break;

      if(i < len) {
        *wstat = w;
        return 1 + i;
      }

      continue;
    }

    break;
  }

  return r;
}
