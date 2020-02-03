#ifndef WAIT_H
#define WAIT_H

#include "uint64.h"
#include "windoze.h"

#ifdef __cplusplus
extern "C" {
#endif


#if WINDOWS_NATIVE
#define WAIT_IF_SIGNALED(status) ((status)&0)
#define WAIT_TERMSIG(status) ((status)&0)
#define WAIT_EXITSTATUS(status) ((status)&0x7f)
#else
#define WAIT_IF_SIGNALED(status) WIFSIGNALED(status)
#define WAIT_TERMSIG(status) WTERMSIG(status)
#define WAIT_EXITSTATUS(status) ((status)&0x7f)
#endif

int wait_pid(int pid, int* wstat);
int wait_pid_nohang(int pid, int* wstat);
int waitpid_nointr(int pid, int* wstat, int flags);
int wait_pids_nohang(int const* pids, unsigned int len, int* wstat);

#ifdef __cplusplus
}
#endif

#endif // WAIT_H
