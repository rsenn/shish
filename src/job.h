#ifndef _JOB_H
#define _JOB_H

#include "../lib/windoze.h"
#include "../lib/buffer.h"
#include "../lib/sig.h"

#define NO_OLDNAMES
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#undef NO_OLDNAMES

#if !WINDOWS_NATIVE
#include <sys/wait.h>
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
  int nproc;
  int level; /* sh_subshell when created: a ( ) or $( ) does not see its parent's jobs */
  unsigned bgnd : 1;      /* was this job backgrounded ("cmd &")? controls whether
                             job_wait() prints a "[N]+ Done ..." banner for it. */
  unsigned pipefail : 1;  /* set -o pipefail was on when it started: job_wait() reports
                             the rightmost non-zero member status */
  unsigned announced : 1; /* has a "Stopped" banner been printed for this stop
                             yet? cleared by job_resume() so the next stop is
                             announced again. */
  struct proc procs[];
};

extern int job_terminal, job_pgrp;
/* set by "wait": job_wait() returns as soon as a trap has run, with job_wait_sig = its signal */
extern int job_wait_interruptible, job_wait_sig;
extern volatile bool job_signaled;

/* set while a signal sent by timeout kills a foreground command: no "signaled" report */
extern volatile int job_quiet;
extern struct job *job_list, **job_pointer;
extern pid_t job_bgpid; /* "$!": pid of the most recently backgrounded command */

/* self-pipe: the SIGCHLD handler does only async-signal-safe work then
   writes one byte here; term_read()'s select() loop wakes on
   job_sigfd[0] and handles the rest from ordinary context. POSIX
   only -- stays -1/-1 on WINDOWS_NATIVE. */
extern int job_sigfd[2];

void job_resume_stopped(void);
struct job* job_first(void);   /* first job of the current subshell level */
void job_discard(int level);  /* drop the jobs of subshells deeper than level */
int job_recall(pid_t pid, int* status); /* status of a process of an already finished job */

#define job_current() (job_pointer && *job_pointer ? *job_pointer : 0)

/* "done" means fully reaped: not running, and not merely stopped
   (Ctrl-Z'd) either. */
#define job_done(j) (!job_running(j) && !job_stopped(j))

struct job* job_bypid(pid_t);
struct job* job_find(const char*);
struct job* job_new(unsigned);
struct job* job_signal(pid_t, int status);

int job_fork(struct job*, union node* node, int bgnd);
int job_wait(struct job*, pid_t pid, int* status);

void job_foreground(struct job*);
void job_free(struct job*);

/* every user-visible job status line goes through job_banner().
   job_print() (used by the "jobs" builtin and job_clean()) picks
   JOB_RUNNING/JOB_DONE/JOB_STOPPED from the job's current state. */
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
void job_terminal_init(void);
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

#endif /* _JOB_H */
