#ifndef JOB_H
#define JOB_H

#include <sys/types.h>
#include <signal.h>
#include "sh.h"

#ifdef HAVE_CONFIG_H
<<<<<<< HEAD
# include "../config.h"
#endif
#ifndef HAVE_SIGSET_T
=======
# include "config.h"
# ifndef HAVE_SIGSET_T
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
typedef int sigset_t;
#endif

#ifdef _WIN32
typedef int pid_t;
#endif

#ifndef WEXITSTATUS
# define WEXITSTATUS(st) ((char)((unsigned char)st & 0xff))
#endif

union node;

struct proc
{
  pid_t    pid;
  int      status;
  char    *cmd;
  sigset_t sigold;
  sigset_t signew;
};

struct job
{
  struct job  *next;
  struct proc *procs;
  unsigned int nproc;
  pid_t        pgrp;
  int          exited:1;
  int          control:1; /* running under job control? */
  int          bgnd:1;
};

extern int job_terminal;
extern int job_pgrp;
extern struct job *job_list;
extern struct job **job_ptr;

struct job *job_new(unsigned int n);  
int job_fork(struct job *job, union node *node, int bg);
int job_wait(struct job *job, int pid, int *status, int options);
void job_status(int pid, int status);
void job_init(void);

#else
#warning "job.h included twice!"
#endif /* JOB_H */
