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
#include "../lib/buffer.h"
#include "../lib/sig.h"

#if WINDOWS_NATIVE
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
  sigset_type sigold;
  sigset_type signew;
};

struct job {
  struct job* next;
  int id;
  pid_t pgrp;
  char* command;
  unsigned done : 1;
  unsigned stopped : 1;
  unsigned control : 1; /* running under job control? */
  unsigned bgnd : 1;
  unsigned int nproc;
  struct proc procs[];
};

extern int job_terminal, job_pgrp;
extern struct job *jobs, **jobptr;

#define job_current() (jobptr && *jobptr ? *jobptr : 0)

struct job* job_new(unsigned int n);
struct job* job_get(int id);
int job_fork(struct job* job, union node* node, int bg);
int job_wait(struct job* job, pid_t pid, int* status);
void job_status(pid_t pid, int status);
void job_init(void);
void job_delete(struct job*);
void job_print(struct job*, buffer*);
struct job* job_find(const char*);
struct job* job_bypid(pid_t id);
void job_delete(struct job*);
int job_done(struct job*);
struct job* job_signal(pid_t pid, int status);

#endif /* _JOB_H */
