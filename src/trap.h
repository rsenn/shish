#ifndef TRAP_H
#define TRAP_H

#include "builtin_config.h"

/* The trap machinery lives in builtin_trap.c. With BUILTIN_TRAP off the calls
 * below are constants, so evaluator, jobs and line editor need no #if.
 * ----------------------------------------------------------------------- */
#if BUILTIN_TRAP

extern int trap_run_count, trap_run_sig; /* traps run so far; the signal of the last one */

void trap_run_pending(void);   /* run the traps whose signal has fired */
void* trap_snapshot_save(void);
void trap_snapshot_restore(void*);
int trap_ignores(int sig);
void trap_reset_caught(void);
int trap_exit(int status);
int trap_exit_running(void);
int trap_exit_pending(void);
int trap_exit_set_here(void);
int trap_return_status(void);
int trap_signal_parent(int sig);

#else

#define trap_run_count 0
#define trap_run_sig 0
#define trap_run_pending() ((void)0)
#define trap_snapshot_save() ((void*)0)
#define trap_snapshot_restore(p) ((void)0)
#define trap_ignores(sig) 0
#define trap_reset_caught() ((void)0)
#define trap_exit(status) (status)
#define trap_exit_running() 0
#define trap_exit_pending() 0
#define trap_exit_set_here() 0
#define trap_return_status() (-1)
#define trap_signal_parent(sig) 0

#endif

#endif /* TRAP_H */
