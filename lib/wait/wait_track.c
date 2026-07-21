#include "../wait.h"
#include "../windoze.h"

#if WINDOWS_NATIVE

/* pids of children spawned via src/fork.c's Windows fork() emulation
 * that haven't been reaped yet -- see the comment in ../wait.h. Fixed
 * size, matching this codebase's other simple static-table limits
 * (e.g. FD_MAX); a shell realistically never has anywhere near this
 * many un-reaped children at once.
 * ----------------------------------------------------------------------- */
#define WAIT_TRACK_MAX 256

static int wait_track_pids[WAIT_TRACK_MAX];
static unsigned int wait_track_n = 0;

void
wait_track_add(int pid) {
  if(wait_track_n < WAIT_TRACK_MAX)
    wait_track_pids[wait_track_n++] = pid;
}

void
wait_track_remove(int pid) {
  unsigned int i;

  for(i = 0; i < wait_track_n; i++) {
    if(wait_track_pids[i] == pid) {
      wait_track_pids[i] = wait_track_pids[--wait_track_n];
      return;
    }
  }
}

unsigned int
wait_track_count(void) {
  return wait_track_n;
}

int
wait_track_pid(unsigned int i) {
  return wait_track_pids[i];
}

#endif
