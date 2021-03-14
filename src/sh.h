#ifndef SH_H
#define SH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "job.h"

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
  unsigned int c;
  char** v;
  int a;          /* arguments alloced? */
  unsigned int s; /* shift count */
};

enum { SH_DEBUG = 0x80, SH_ERREXIT = 0x40, SH_NOINTERACTIVE = 0x1000 };

struct env {
  struct env* parent;
  stralloc cwd;
  int cwdsym; /* is cwd symbolic or phyiscal? */
  long umask;
  int flags;
  int exitcode; /* exit code of last evaluated tree */
  struct fdstack* fdstack;
  struct vartab* varstack;
  jmp_buf jmpbuf;
  int jump;
  struct arg arg;
  struct parser* parser;
  struct eval* eval;
};

extern int sh_argc;    /* initial argument count */
extern char** sh_argv; /*    "       "     vector */
extern char** sh_envp; /*    "    environment */
extern char* sh_name;
extern char* sh_argv0;
extern int sh_child;

extern struct env* sh;

extern struct env sh_root;
extern const char* sh_home;
extern uid_t sh_uid;
extern pid_t sh_pid;

union node;

int sh_error(const char* s);
int sh_errorn(const char* s, unsigned int len);
void sh_exit(int retcode);
int sh_forked(void);
void sh_getcwd(struct env* sh);
const char* sh_gethome(void);
void sh_init(void);
void sh_loop(void);
int sh_main(int argc, char** argv, char** envp);
void sh_msg(char* s);
void sh_msgn(char* s, size_t n);
int sh_pop(struct env* env);
void sh_popargs(struct arg* arg);
void sh_push(struct env* env);
void sh_pushargs(struct arg* arg);
void sh_setargs(char** argv, int dup);
int sh_subshell(union node* list, int flags);
void sh_usage(void);

#endif /* SH_H */
