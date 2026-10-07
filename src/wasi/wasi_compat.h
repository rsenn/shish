#ifndef WASI_COMPAT_H
#define WASI_COMPAT_H

/* wasi-libc omits process, terminal and signal-mask APIs. These stubs let
 * the shell compile; each fails the way a system without the feature does.
 * Force-included on WASI builds only (see CMakeLists.txt).
 * ----------------------------------------------------------------------- */
#ifdef __wasi__

#include <errno.h>
#include <stddef.h>
#include <signal.h>
#include <sys/types.h>

/* processes: none exist, so fork/exec/wait always fail */
#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(s) (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define WIFSIGNALED(s) (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFSTOPPED(s) (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s) (((s) >> 8) & 0xff)

static inline pid_t
fork(void) {
  errno = ENOSYS;
  return -1;
}

static inline pid_t
waitpid(pid_t pid, int* status, int options) {
  (void)pid, (void)status, (void)options;
  errno = ECHILD;
  return -1;
}

static inline int
execve(const char* path, char* const argv[], char* const envp[]) {
  (void)path, (void)argv, (void)envp;
  errno = ENOSYS;
  return -1;
}

static inline int
kill(pid_t pid, int sig) {
  (void)pid;
  return sig ? raise(sig) : 0;
}

static inline int
mkstemp(char* tmpl) {
  errno = ENOSYS;
  return -1;
}

/* identity and file mode: one anonymous user */
static inline unsigned
getuid(void) {
  return 0;
}

static inline unsigned
getgid(void) {
  return 0;
}

static inline unsigned
geteuid(void) {
  return 0;
}

static inline unsigned
getegid(void) {
  return 0;
}

static inline int
setuid(unsigned uid) {
  return uid ? (errno = EPERM, -1) : 0;
}

static inline int
setgid(unsigned gid) {
  return gid ? (errno = EPERM, -1) : 0;
}

static inline unsigned
umask(unsigned mask) {
  static unsigned cur = 022;
  unsigned old = cur;

  cur = mask & 0777;
  return old;
}

static inline int
killpg(pid_t pg, int sig) {
  return kill(pg, sig);
}

static inline pid_t
wait3(int* status, int options, void* rusage) {
  return waitpid(-1, status, options);
}

/* signal masks: a single-threaded module has nothing to block */
#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2
#define SA_RESTART 0x10000000
#define SA_SIGINFO 0x4
#define SA_NOCLDSTOP 0x1

typedef struct {
  pid_t si_pid;
} siginfo_t;

struct sigaction {
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
  };
  sigset_t sa_mask;
  int sa_flags;
};

/* an SA_SIGINFO handler runs through a one-argument trampoline: raise() has no sender */
static void (*wasi_siginfo[64])(int, siginfo_t*, void*);

static inline void
wasi_siginfo_tramp(int sig) {
  wasi_siginfo[sig & 63](sig, NULL, NULL);
}

static inline int
sigemptyset(sigset_t* s) {
  *s = 0;
  return 0;
}

static inline int
sigfillset(sigset_t* s) {
  *s = ~0UL;
  return 0;
}

static inline int
sigaddset(sigset_t* s, int sig) {
  *s |= 1UL << (sig & 31);
  return 0;
}

static inline int
sigismember(const sigset_t* s, int sig) {
  return (*s >> (sig & 31)) & 1;
}

static inline int
sigprocmask(int how, const sigset_t* set, sigset_t* old) {
  (void)how, (void)set;
  if(old)
    *old = 0;
  return 0;
}

static inline int
sigaction(int sig, const struct sigaction* act, struct sigaction* old) {
  void (*h)(int) = act ? act->sa_handler : SIG_DFL;
  void (*prev)(int);

  if(act && (act->sa_flags & SA_SIGINFO)) {
    wasi_siginfo[sig & 63] = act->sa_sigaction;
    h = wasi_siginfo_tramp;
  }
  prev = signal(sig, h);

  if(!act)
    signal(sig, prev);
  if(old) {
    old->sa_handler = prev == SIG_ERR ? SIG_DFL : prev;
    old->sa_mask = 0;
    old->sa_flags = 0;
  }
  return prev == SIG_ERR ? -1 : 0;
}

/* users: a single anonymous user, no passwd database */
struct passwd {
  char* pw_name;
  char* pw_passwd;
  unsigned pw_uid, pw_gid;
  char* pw_gecos;
  char* pw_dir;
  char* pw_shell;
};

static inline struct passwd*
getpwnam(const char* name) {
  (void)name;
  return NULL;
}

static inline struct passwd*
getpwuid(unsigned uid) {
  (void)uid;
  return NULL;
}

struct group {
  char* gr_name;
  char* gr_passwd;
  unsigned gr_gid;
  char** gr_mem;
};

static inline struct group*
getgrgid(unsigned gid) {
  (void)gid;
  return NULL;
}

static inline struct group*
getgrnam(const char* name) {
  (void)name;
  return NULL;
}

static inline int
getpwuid_r(unsigned uid, struct passwd* pw, char* buf, size_t len, struct passwd** res) {
  (void)uid, (void)pw, (void)buf, (void)len;
  *res = NULL;
  return 0;
}

static inline int
getgrgid_r(unsigned gid, struct group* gr, char* buf, size_t len, struct group** res) {
  (void)gid, (void)gr, (void)buf, (void)len;
  *res = NULL;
  return 0;
}

/* no working directory to change, no time zone database */
static inline int
fchdir(int fd) {
  (void)fd;
  errno = ENOSYS;
  return -1;
}

static inline void
tzset(void) {
}

/* interval timers: SIGALRM cannot be delivered, so there is nothing to arm */
#define ITIMER_REAL 0

struct itimerval {
  struct {
    long tv_sec, tv_usec;
  } it_interval, it_value;
};

static inline int
setitimer(int which, const struct itimerval* v, struct itimerval* old) {
  (void)which, (void)v, (void)old;
  errno = ENOSYS;
  return -1;
}

/* terminal: never a tty */
typedef unsigned tcflag_t;
typedef unsigned char cc_t;
typedef unsigned speed_t;
#define NCCS 32

struct termios {
  tcflag_t c_iflag, c_oflag, c_cflag, c_lflag;
  cc_t c_line, c_cc[NCCS];
};

#define ISIG 0x1
#define ICANON 0x2
#define ECHO 0x8
#define ECHOE 0x10
#define ECHOK 0x20
#define ECHONL 0x40
#define IEXTEN 0x8000
#define ICRNL 0x100
#define IXON 0x400
#define OPOST 0x1
#define VMIN 6
#define VTIME 5
#define VINTR 0
#define VEOF 4
#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

static inline int
tcgetattr(int fd, struct termios* t) {
  (void)fd, (void)t;
  errno = ENOTTY;
  return -1;
}

static inline int
tcsetattr(int fd, int act, const struct termios* t) {
  (void)fd, (void)act, (void)t;
  errno = ENOTTY;
  return -1;
}

static inline pid_t
tcgetpgrp(int fd) {
  (void)fd;
  errno = ENOTTY;
  return -1;
}

static inline int
tcsetpgrp(int fd, pid_t pg) {
  (void)fd, (void)pg;
  errno = ENOTTY;
  return -1;
}

static inline int
setpgid(pid_t pid, pid_t pg) {
  (void)pid, (void)pg;
  errno = ENOSYS;
  return -1;
}

static inline pid_t
getpgrp(void) {
  return 1;
}

#endif /* __wasi__ */
#endif /* WASI_COMPAT_H */
