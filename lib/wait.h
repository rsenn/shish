#ifndef _WAIT_H
#define _WAIT_H

#include "uint64.h"
#include "windoze.h"

#if WINDOWS_NATIVE
#define WAIT_IF_SIGNALED(status) ((status)&0)
#define WAIT_TERMSIG(status) ((status)&0)
#define WAIT_EXITSTATUS(status) ((status)&0x7f)
#else
#define WAIT_IF_SIGNALED(status) WIFSIGNALED(status)
#define WAIT_TERMSIG(status) WTERMSIG(status)
#define WAIT_EXITSTATUS(status) ((status)&0x7f)
#endif

#define WAIT_IF_EXITED(status) (WTERMSIG(status) == 0)
#define WAIT_IF_STOPPED(status) (((status)&0xff) == 0x7f)

int wait_nohang(int* wstat);
int wait_pid(int pid, int* wstat);
int wait_pid_nohang(int pid, int* wstat);
int waitpid_nointr(int pid, int* wstat, int flags);
int wait_pids_nohang(int const* pids, unsigned int len, int* wstat);

#endif // _WAIT_H
