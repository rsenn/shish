#ifndef WAIT_H
#define WAIT_H

#include "uint64.h"

#ifdef __cplusplus
extern "C" {
#endif

int wait_pid(int pid, int* wstat);
int wait_pid_nohang(int pid, int* wstat);
int wait_pids_nohang(int const* pids, unsigned int len, int* wstat);

#ifdef __cplusplus
}
#endif

#endif // WAIT_H
