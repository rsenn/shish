#include "../wait.h"

/* drains wait_nohang() (wait for *any* child, non-blocking) until it
 * reports the specific pid we're after, discarding any other
 * children reaped along the way. wait_nohang() already has a
 * WINDOWS_NATIVE implementation, so this needs none of its own --
 * the old WINDOWS_NATIVE branch that used to live in this file
 * duplicated wait_nohang()'s OpenProcess()/WaitForSingleObject(0)
 * logic instead of just calling it.
 * ----------------------------------------------------------------------- */
int
wait_pid_nohang(int pid, int* wstat) {
  int w = 0;
  int r = 0;

  while(r != pid) {
    r = wait_nohang(&w);

    if(!r || (r == (int)-1))
      return r;
  }
  *wstat = w;
  return r;
}
