#ifndef SH_H
#define SH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "job.h"

#include "../lib/uint16.h"
#include "../lib/stralloc.h"
#include "../lib/windoze.h"
#if WINDOWS_NATIVE
#ifndef HAVE_UID_T
typedef int uid_t;
#endif
#endif

#include <setjmp.h>
#include <stdlib.h>
#ifdef __TINYC__
#define NO_OLDNAMES
#endif
#include <sys/types.h>

struct eval;
struct fdtable;
struct vartab;

/*#define SH_INTERACTIVE 0x0001*/

struct arg {
  char** v;
  int c;
  int a; /* arguments alloced? */
  int s; /* shift count */
};

/*enum {
  SH_UNSET = 0x08,
  SH_NOCLOBBER = 0x10,
  SH_DEBUG = 0x80,
  SH_ERREXIT = 0x40,
  SH_NOINTERACTIVE = 0x1000
};*/

/*union shopt {
  unsigned flags : 5;*/
struct shopt {
  unsigned allexport : 1;   /* -a */
  unsigned errexit : 1;     /* -e */
  unsigned noglob : 1;      /* -f */
  unsigned hashall : 1;     /* -h */
  unsigned monitor : 1;     /* -m */
  unsigned noexec : 1;      /* -n */
  unsigned privileged : 1;  /* -p */
  unsigned unset : 1;       /* -u */
  unsigned xtrace : 1;      /* -x */
  unsigned braceexpand : 1; /* -B */
  unsigned noclobber : 1;   /* -C */
  unsigned histexpand : 1;  /* -H */
};

/* name->letter table for every "set"-supported option, and the
 * apply/get dispatch built on it -- see builtin_set.c, which owns the
 * actual table contents/switch bodies; declared here so sh_main.c can
 * give the shell's own command-line startup options the identical
 * letter/name set without duplicating (and risking drifting from)
 * that list a second time. */
struct set_longopt {
  const char* name;
  char letter;
};

extern const struct set_longopt set_longopts[];
extern const size_t set_longopts_n;

int set_apply(struct shopt* opts, int letter, int on);
int set_get(const struct shopt* opts, int letter);
/*};*/

typedef void handler_fn(void);

struct handler {
  struct handler* next;
  handler_fn* fn;
};

struct env {
  struct env* parent;
  stralloc cwd;
  unsigned cwdsym : 1; /* is cwd symbolic or phyiscal? */
  unsigned umask : 12;
  short exitcode;            /* exit code of last evaluated tree */
  unsigned cmdsubst_ran : 1; /* did the most recent word expansion run a
                                command substitution? see eval_simple_command's
                                "no command, only assignments" status handling */
  struct shopt opts;
  struct fdstack* fdstack;
  struct vartab* varstack;
  struct arg arg;
  struct parser* parser;
  struct eval* eval;
  struct handler* finalizers;
};

extern int sh_argc;    /* initial argument count */
extern char** sh_argv; /*    "       "     vector */
extern char** sh_envp; /*    "    environment */
extern const char* sh_name;
extern char* sh_argv0;
extern int sh_child;

/* whether this shell session is interactive, decided once at startup
   (sh_main.c) -- distinct from a `struct source`'s own
   SOURCE_IACTIVE bit (source.h), which source_push() resets to 0 for
   every nested source (a `.`-sourced file, a here-doc, ...) and which
   correctly gates per-buffer things like prompting/history. POSIX's
   "a non-interactive shell exits on this error" rules (2.8.1's
   assignment/redirection/special-builtin errors, 2.6.1's unset-
   parameter error, 2.11's signal-ignored-on-entry) are a property of
   the whole session, not of whichever buffer happens to be open right
   now -- checking SOURCE_IACTIVE for these would wrongly make a
   plain command failing inside a `.`-sourced file kill an otherwise
   interactive shell the moment it's one level into any sourced file. */
extern int sh_interactive;

/* set while a real-signal trap's body is running (trap_handler(),
 * builtin_trap.c) and it calls "exit" -- see eval_subshell.c's own
 * comment for why. A trap fires asynchronously, possibly while deep
 * inside an in-process subshell ("(...)" doesn't fork here), and its
 * "exit" must terminate the whole process regardless of that, not
 * just whatever subshell/eval happened to be active at the exact
 * moment the signal arrived -- unlike an *ordinary*, synchronous
 * "exit" naturally written inside a "(...)" in the script itself,
 * which correctly stays scoped to just that subshell. */
extern int sh_async_exit;

extern struct env* sh;

extern struct env sh_root;
extern const char* sh_home;
extern uid_t sh_uid;
/* sh_pid is *this* process's real OS pid -- it changes across fork()
   (sh_forked() updates it) because job control (setpgid/tcsetpgrp) and
   /proc/<pid>/fd/ paths need the actual pid of whichever process is
   running right now. sh_shpid is "$$": POSIX defines it as the pid of
   the originally invoked shell, fixed for the whole script no matter
   how many pipeline/subshell/cmdsubst forks happen along the way, so
   it's set once at startup and never touched again. */
extern pid_t sh_pid;
extern pid_t sh_shpid;

union node;

int sh_error(const char* s);
int sh_errorn(const char* s, unsigned int len);
int sh_error_errno(const char* s);
int sh_errorn_errno(const char* s, unsigned int len);
void sh_exit(int retcode);
size_t sh_fmtflags(char* dest, const struct shopt*);
int sh_forked(void);
void sh_getcwd(struct env* sh);
const char* sh_gethome(void);
void sh_init(void);
void sh_loop(void);
int sh_main(int argc, char** argv, char** envp);
void sh_msg(const char* s);
void sh_msgn(const char* s, size_t n);
int sh_pop(struct env* env);
void sh_popargs(struct arg* arg);
void sh_push(struct env* env);
void sh_pushargs(struct arg* arg);
void sh_setargs(char** argv, int dup);
void sh_source(const char* path);
size_t sh_unescape(const char* src, size_t len, char* dst);
void sh_usage(void);

#endif /* SH_H */
