#ifndef _JOB_H
#define _JOB_H

#include <signal.h>

#ifdef __TINYC__
#ifdef _WIN32
#define NO_OLDNAMES
#define pid_t _pid_t
#endif
#endif
#include <sys/types.h>
#undef NO_OLDNAMES

#ifndef _WIN32
#include <sys/wait.h>
#endif

#include "../lib/windoze.h"

#if WINDOWS_NATIVE
typedef int sigset_t;
#if !defined(HAVE_PID_T) && !defined(__MINGW32__) && !defined(__TINYC__)
typedef int pid_t;
#endif
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(st) ((char)((unsigned char)st & 0xff))
#endif

union node;

struct proc {
  pid_t pid;
  int status;
  char* cmd;
  sigset_t sigold;
  sigset_t signew;
};

struct job {
  struct job* next;
  struct proc* procs;
  unsigned int nproc;
  pid_t pgrp;
  unsigned exited : 1;
  unsigned control : 1; /* running under job control? */
  unsigned bgnd : 1;
};

extern int job_terminal;
extern int job_pgrp;
extern struct job* job_list;
extern struct job** job_ptr;

struct job* job_new(unsigned int n);
int job_fork(struct job* job, union node* node, int bg);
int job_wait(struct job* job, int pid, int* status);
void job_status(int pid, int status);
void job_init(void);

#endif /* _JOB_H */
