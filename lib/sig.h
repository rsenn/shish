#ifndef _SIG_H
#define _SIG_H

#include <sys/types.h>
#include <signal.h>
#include "windoze.h"

#if WINDOWS_NATIVE
typedef long sigset_t;

#ifndef SIG_BLOCK
#define SIG_BLOCK 1
#endif /* defined(SIG_BLOCK) */
#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 2
#endif /* defined(SIG_UNBLOCK) */

#ifndef SIGALL
#define SIGALL (~(sigset_t)0L) /* All signals.    */
#endif

#ifndef sigbit
#define sigbit(n) (1L << ((n)-1))
#endif
#ifndef sigemptyset
#define sigemptyset(s) *(s) = ~SIGALL
#endif
#ifndef sigfillset
#define sigfillset(s) *(s) = SIGALL
#endif

#ifndef sigaddset
#define sigaddset(s, n) *(s) |= sigbit(n)
#endif
#ifndef sigdelset
#define sigdelset(s, n) *(s) &= ~sigbit(n)
#endif
#ifndef sigismember
#define sigismember(set, n) ((*(set)&sigbit(n)) == sigbit(n))
#endif
#include <errno.h>
#ifndef ENOBUFS
#define ENOBUFS 1039
#endif
#endif

#define SA_MASKALL 1

#ifndef SA_NOCLDSTOP
#define SA_NOCLDSTOP 2
#endif

typedef void sighandler_t_fn(int);
typedef sighandler_t_fn* sighandler_t_ref;

#if !defined(_POSIX_SOURCE) && !defined(__linux__) && !defined(__unix__) && !defined(__wasi__)
struct sigaction {
  sighandler_t_ref sa_handler;
  unsigned int sa_flags : 2;
};
#endif

extern struct sigaction const sig_dfl;
extern struct sigaction const sig_ign;

#ifndef SA_MASKALL
#define SA_MASKALL ((unsigned long)0x01)
#endif
#ifndef SA_NOCLDSTOP
#define SA_NOCLDSTOP ((unsigned long)0x02)
#endif

#define SIGSTACKSIZE 16

#define sig_catcha(sig, ac) sig_action(sig, (ac), 0)
#define sig_restore(sig) sig_action((sig), &sig_dfl, 0)

int sig_action(int sig, struct sigaction const* new, struct sigaction* old);
void sig_block(int);
void sig_blocknone(void);
void sig_blockset(const void*);
int sig_catch(int, sighandler_t_ref);
int sigfpe(void);
int sig_ignore(int);
char const* sig_name(int);
int sig_number(char const*);
void sig_pause(void);
int sig_push(int, sighandler_t_ref);
int sig_pusha(int sig, struct sigaction const* ssa);
void sig_restoreto(const void*, unsigned int);
int sigsegv(void);
void sig_shield(void);
void sig_unshield(void);

#endif
