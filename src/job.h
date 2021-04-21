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
  sigset_t sigold;
  sigset_t signew;
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

struct job* job_new(unsigned int n);
struct job* job_get(int id);
int job_fork(struct job* job, union node* node, int bg);
int job_wait(struct job* job, int pid, int* status);
void job_status(int pid, int status);
void job_init(void);
void job_delete(struct job*);
void job_print(struct job*, buffer*);
struct job* job_find(const char*);
void job_delete(struct job*);

#endif /* _JOB_H */
