#ifndef JOB_H
#define JOB_H

#include <signal.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif

#include "windoze.h"
#if WINDOWS_NATIVE
typedef int sigset_t;
#ifndef HAVE_PID_T
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
  int exited : 1;
  int control : 1; /* running under job control? */
  int bgnd : 1;
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

#else
#warning "job.h included twice!"
#endif /* JOB_H */
