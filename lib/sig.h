#ifndef _SIG_H
#define _SIG_H

#include <sys/types.h>
#include <signal.h>
#include "windoze.h"

#if WINDOWS_NATIVE
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGURG 23
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGIO 29
#define SIGPWR 30
#define SIGSYS 31
#endif

#ifndef SIG_BLOCK
#define SIG_BLOCK 1
#endif

#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 2
#endif

#include <errno.h>

#ifndef ENOBUFS
#define ENOBUFS 1039
#endif

typedef void sighandler_t_fn(int);
typedef sighandler_t_fn* sighandler_t_ref;

#ifndef WTERMSIG
#define WTERMSIG(status) ((status) & 0x7f)
#endif

#ifndef WIFSTOPPED
#define WIFSTOPPED(status) (((status) & 0xff) == 0x7f)
#endif

#if (!defined(_POSIX_SOURCE) && !defined(__linux__) && !defined(__unix__) && \
     !defined(__wasi__)) || \
    (defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__))
typedef unsigned long long sigset_t;

struct sigaction {
  sighandler_t_ref sa_handler;
  sigset_t sa_mask;
  unsigned int sa_flags;
  void (*sa_restorer)(void);
};
#endif

typedef unsigned long sigset_type;

#ifndef SA_MASKALL
#define SA_MASKALL ((unsigned long)0x01)
#endif
#ifndef SA_NOCLDSTOP
#define SA_NOCLDSTOP ((unsigned long)0x02)
#endif

#define SIGSTACKSIZE 16

/* used internally by sig_catch.c */
#define sig_catcha(sig, ac) sig_action(sig, (ac), 0)

int sig_action(int sig, struct sigaction const* new, struct sigaction* old);
void sig_block(int);
void sig_unblock(int);
void sig_blocknone(void);
int sig_catch(int, sighandler_t_ref);
const char* sig_name(int);
int sig_byname(const char* name);
int sig_number(const char*);
int sig_push(int, sighandler_t_ref);
int sig_pusha(int sig, struct sigaction const* ssa);
int sig_pop(int sig);

#endif
