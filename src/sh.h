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
  unsigned notify : 1;      /* -b */
  unsigned verbose : 1;     /* -v */
  unsigned pipefail : 1;    /* -o pipefail (no letter) */
  unsigned ignoreeof : 1;   /* -o ignoreeof (no letter): sh_loop() answers an interactive end-of-file with a reminder */
  unsigned nolog : 1;       /* -o nolog (no letter): accepted */
  unsigned vi : 1;          /* -o vi (no letter): accepted */
};

/* name->letter table of every "set" option (owned by builtin_set.c), shared so that
 * sh_main.c's startup options use the identical letter/name set */
struct set_longopt {
  const char* name;
  char letter;
};

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
  struct arg arg;

  struct fdstack* fdstack;
  struct vartab* varstack;
  struct parser* parser;
  struct eval* eval;
  struct handler* finalizers;
};

extern const struct set_longopt set_longopts[];
extern const size_t set_longopts_n;

extern int sh_argc;    /* initial argument count */
extern char** sh_argv; /*    "       "     vector */
extern char** sh_envp; /*    "    environment */
extern const char* sh_name;
extern char* sh_argv0;
extern int sh_child;

/* the whole session is interactive, decided once at startup. Not source->mode & SOURCE_IACTIVE,
 * which resets per nested source: POSIX's "a non-interactive shell exits on this error" rules
 * (2.8.1, 2.6.1, 2.11) are a property of the session. */
extern int sh_interactive;

/* set while a trap body runs "exit" (trap_handler(), builtin_trap.c): the exit must end the
 * whole process, not just the in-process subshell the asynchronous signal interrupted */
extern int sh_async_exit;

extern struct env* sh;

extern struct env sh_root;
extern const char* sh_home;
extern uid_t sh_uid;

/* sh_pid: this process's real pid (sh_forked() updates it: job control, /proc paths).
 * sh_shpid: "$$", the original shell's pid, set once at startup */
extern pid_t sh_pid;
extern pid_t sh_shpid;

extern int sh_async;
extern int sh_subshell; /* nesting of in-process ( ) and $( ) */

union node;

int sh_utf8(void); /* do LC_ALL/LC_CTYPE/LANG ask for UTF-8 text handling? (src/sh/sh_utf8.c) */
int sh_error(const char* s);
int sh_errorn(const char* s, unsigned int len);
int sh_error_errno(const char* s);
int sh_errorn_errno(const char* s, unsigned int len);
void sh_exit(int retcode);
int sh_child_exit(int status); /* run the EXIT trap a forked child set, then pass status on */
size_t sh_fmtflags(char* dest, const struct shopt*);
int sh_forked(void);
void sh_sigignore(void);  /* interactive shell ignores INT QUIT TERM */
void sh_sigrestore(void);
void sh_sigasync(void);   /* async list without job control: ignore INT QUIT */ /* ...children get the defaults back */
void sh_getcwd(struct env* sh);
const char* sh_gethome(void);
void sh_init(void);
void sh_loop(void);
int sh_main(int argc, char** argv, char** envp);
void sh_msg(const char* s);
/* where the simple command being run began; diagnostics name this line, not the parser's position */
extern struct location sh_errloc;
extern int sh_errloc_set;
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
