#ifndef _WAIT_H
#define _WAIT_H

#include "uint64.h"
#include "windoze.h"

#if WINDOWS_NATIVE
#define WAIT_IF_SIGNALED(status) ((status) & 0)
#define WAIT_TERMSIG(status) ((status) & 0)
#define WAIT_EXITSTATUS(status) (((status) >> 8) & 0xff)
#else
#define WAIT_IF_SIGNALED(status) WIFSIGNALED(status)
#define WAIT_TERMSIG(status) WTERMSIG(status)
#define WAIT_EXITSTATUS(status) (((status) >> 8) & 0xff)
#endif

#define WAIT_IF_EXITED(status) (WTERMSIG(status) == 0)
#define WAIT_IF_STOPPED(status) (((status) & 0xff) == 0x7f)

int wait_nohang(int* wstat);
int wait_pid(int pid, int* wstat);
int wait_pid_nohang(int pid, int* wstat);
int waitpid_nointr(int pid, int* wstat, int flags);
int wait_pids_nohang(int const* pids, unsigned int len, int* wstat);

#if WINDOWS_NATIVE
/* Windows has no equivalent of POSIX's "wait for any child of this
 * process" (the kernel doesn't track a parent/child relationship the
 * way it does on unix) -- fork() in src/fork.c is the sole producer
 * of child processes here, and registers each one's pid via
 * wait_track_add() right after creating it, so wait_nohang() has
 * something to enumerate. wait_pid()/wait_pid_nohang()/
 * wait_pids_nohang() call wait_track_remove() once they've reaped a
 * given pid directly, so the registry doesn't accumulate stale
 * entries for children reaped some other way than wait_nohang().
 */
void wait_track_add(int pid);
void wait_track_remove(int pid);
unsigned int wait_track_count(void);
int wait_track_pid(unsigned int i);
#endif

#endif // _WAIT_H
