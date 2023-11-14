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
extern struct job *jobs, **jobptr;

#define job_current() (jobptr && *jobptr ? *jobptr : 0)

int job_done(struct job*);
int job_fork(struct job*, union node* node, int bgnd);
int job_wait(struct job*, pid_t pid, int* status);
struct job* job_bypid(pid_t);
struct job* job_find(const char*);
struct job* job_get(int);
struct job* job_new(unsigned int);
struct job* job_signal(pid_t, int status);
void job_clean(void);
void job_free(struct job*);
void job_dump(buffer*);
void job_foreground(struct job*);
void job_init(void);
void job_print(struct job*, buffer* out);
void job_status(pid_t, int status);

#endif /* _JOB_H */
