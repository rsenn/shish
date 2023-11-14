#ifndef _JOB_H
#define _JOB_H

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>

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
  sigset_type signals;
};

struct job {
  struct job* next;
  int id;
  pid_t pgrp;
  char* command;
  uint8_t nproc;
  struct proc procs[];
};

extern int job_terminal, job_pgrp;
extern volatile bool job_signaled;
 extern struct job *job_list, **job_pointer;

#define job_current() (job_pointer && *job_pointer ? *job_pointer : 0)
#define job_done(j) (!job_running(j))

struct job* job_bypid(pid_t);
struct job* job_find(const char*);
struct job* job_get(int);
struct job* job_new(unsigned);
struct job* job_signal(pid_t, int status);

int job_fork(struct job*, union node* node, int bgnd);
int job_wait(struct job*, pid_t pid, int* status);

void job_foreground(struct job*);
void job_free(struct job*);
void job_print(struct job*, buffer* out);

void job_clean(void);
void job_dump(buffer*);
void job_init(void);
void job_printstatus(pid_t, int status);
void job_update(void);

static inline bool
job_running(struct job* j) {
  for(size_t i = 0; i < j->nproc; i++)
    if(j->procs[i].status == -1)
      return true;

  return false;
}

static inline bool
job_stopped(struct job* j) {
  for(size_t i = 0; i < j->nproc; i++)
    if(j->procs[i].status != -1)
      if(WIFSTOPPED(j->procs[i].status))
        return true;

  return false;
}

static inline struct proc*
job_proc(struct job* j, pid_t pid) {
  for(size_t i = 0; i < j->nproc; i++)
    if(j->procs[i].pid == pid)
      return &j->procs[i];

  return 0;
}

static inline struct proc*
proc_bypid(pid_t pid) {
  struct job* j;

  if((j = job_bypid(pid)))
    return job_proc(j, pid);

  return 0;
}

#endif /* _JOB_H */
