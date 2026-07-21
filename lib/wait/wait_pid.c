#include "../wait.h"

/* just a blocking wait for a specific pid -- waitpid_nointr() already
 * has a WINDOWS_NATIVE implementation of exactly that, no need to
 * duplicate it here (the old WINDOWS_NATIVE branch that used to live
 * in this file even disagreed with waitpid_nointr() on what to
 * return on success: 1, not pid)
 * ----------------------------------------------------------------------- */
int
wait_pid(int pid, int* wstat) {
  return waitpid_nointr(pid, wstat, 0);
}
