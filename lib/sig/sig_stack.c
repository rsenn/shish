#include "../sig.h"

#include <errno.h>
#include <signal.h>

#ifdef SHISH_NSIG
static struct sigaction sig_stack[SHISH_NSIG - 1][SIGSTACKSIZE];
static unsigned int sigsp[SHISH_NSIG - 1];

int
sig_pusha(int sig, struct sigaction const* ssa) {
  if((sig <= 0) || (sig >= SHISH_NSIG))
    return (errno = EINVAL, -1);

  if(sigsp[sig - 1] >= SIGSTACKSIZE)
    return (errno = ENOBUFS, -1);

  if(sig_action(sig, ssa, &sig_stack[sig - 1][sigsp[sig - 1]]) == -1)
    return -1;

  return ++sigsp[sig - 1];
}

int
sig_pop(int sig) {
  if((sig <= 0) || (sig >= SHISH_NSIG))
    return (errno = EINVAL, -1);

  if(!sigsp[sig - 1])
    return (errno = EFAULT, -1);

  if(sig_action(sig, &sig_stack[sig - 1][sigsp[sig - 1] - 1], 0) == -1)
    return -1;

  return --sigsp[sig - 1];
}
#endif
