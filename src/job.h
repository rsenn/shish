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
#define WEXITSTATUS(st) ((unsigned char)((st) >> 8))
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
  uint8_t bgnd; /* was this job actually backgrounded ("cmd &")? job_wait()'s
                   "[N]+ Done ..." banner is only for these -- a foreground
                   pipeline (including one run internally to capture a
                   command substitution's output) isn't something the user
                   is waiting to be notified about; they're already watching
                   it (or, for a substitution, it was never visible at all). */
  struct proc procs[];
};

extern int job_terminal, job_pgrp;
extern volatile bool job_signaled;
extern struct job *job_list, **job_pointer;
extern pid_t job_bgpid; /* "$!": pid of the most recently backgrounded command */

#define job_current() (job_pointer && *job_pointer ? *job_pointer : 0)
/* "done" means fully reaped -- a job with a stopped (Ctrl-Z'd) process
   isn't running (job_running() only looks for status == -1, unset)
   but isn't done either; without excluding job_stopped() here, a job
   that just stopped would look "done" and get job_free()'d instead of
   staying in job_list for a later "fg"/"bg" to resume */
#define job_done(j) (!job_running(j) && !job_stopped(j))

struct job* job_bypid(pid_t);
struct job* job_find(const char*);
struct job* job_get(int);
struct job* job_new(unsigned);
struct job* job_signal(pid_t, int status);

int job_fork(struct job*, union node* node, int bgnd);
int job_wait(struct job*, pid_t pid, int* status);

void job_foreground(struct job*);
void job_free(struct job*);

/* every user-visible "the shell is telling you something about a job"
   line shares one formatter, job_banner() -- see src/job/job_banner.c
   for the rationale. job_print() (used by the "jobs" builtin and
   job_clean()'s listing) is just JOB_RUNNING/JOB_DONE/JOB_STOPPED
   auto-selected from the job's current state. */
enum job_banner_kind {
  JOB_START,   /* "[id] pid" -- a job was just forked/backgrounded */
  JOB_RUNNING, /* "[id]+  Running   command" */
  JOB_DONE,    /* "[id]+  Done      command" */
  JOB_STOPPED, /* "[id]+  Stopped   command" */
  JOB_BGRESUME /* "[id]+ command &" -- bg resumed a stopped job */
};

void job_banner(struct job*, buffer* out, enum job_banner_kind);
void job_print(struct job*, buffer* out);

void job_clean(bool);
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
