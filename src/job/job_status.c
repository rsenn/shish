#include "../../lib/windoze.h"
#include "fd.h"
#include "job.h"
#include "../../lib/wait.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#elif defined(__MSYS__) || defined(__CYGWIN__)
#define HAVE_SYS_SIGLIST 1
#endif
#if !defined(HAVE_SYS_SIGLIST)
extern const char* const sys_siglist[];
#endif

/* outputs job status stuff
 * ----------------------------------------------------------------------- */
void
job_status(int pid, int status) {
  if(WAIT_IF_SIGNALED(status)) {
    const char* signame = sys_siglist[WAIT_TERMSIG(status)];

    if(pid) {
      buffer_put(fd_err->w, "process ", 8);
      buffer_putulong(fd_err->w, pid);
      buffer_put(fd_err->w, " signaled: ", 11);
    }

    buffer_putc(fd_err->w, signame[0] + 0x20);
    buffer_puts(fd_err->w, &signame[1]);

#ifdef WCOREDUMP
    if(WCOREDUMP(status)) {
      buffer_putspace(fd_err->w);
      buffer_put(fd_err->w, "(core dumped)", 13);
    }
#endif /* WCOREDUMP */

    buffer_putnlflush(fd_err->w);

    exit(666);
  }
}

#ifndef HAVE_SYS_SIGLIST
const char* const sys_siglist[] = {"Signal 0",
                                   "Hangup",
                                   "Interrupt",
                                   "Quit",
                                   "Illegal instruction",
                                   "Trace/breakpoint trap",
                                   "ABRT/IOT trap",
#if defined(__alpha__) || defined(__sparc__) || defined(__mips__) || defined(__hppa__)
                                   "EMT trap",
#else
                                   "Bus error",
#endif
                                   "Floating point exception",
                                   "Killed",
#if defined(__alpha__) || defined(__sparc__) || defined(__mips__) || defined(__hppa__)
                                   "Bus error",
#else
                                   "User defined signal 1",
#endif
                                   "Segmentation fault",
#if defined(__alpha__) || defined(__sparc__) || defined(__mips__) || defined(__hppa__)
                                   "Bad system call",
#else
                                   "User defined signal 2",
#endif
                                   "Broken pipe",
                                   "Alarm clock",
                                   "Terminated",
#if defined(__hppa__)
                                   "User defined signal 1",
                                   "User defined signal 2",
                                   "Child exited",
                                   "Power lost",
                                   "Virtual timer expired",
                                   "Profiling timer expired",
                                   "I/O possible",
                                   "Window changed",
                                   "Stopped (signal)",
                                   "Stopped",
                                   "Continued",
                                   "Stopped (tty input)",
                                   "Stopped (tty output)",
                                   "Urgent I/O condition",
                                   "Power lost",
                                   "Unknown",
                                   "Unknown",
                                   "CPU time limit exceeded",
                                   "File size limit exceeded",
                                   "Unknown",
                                   "Stack fault",
#elif defined(__mips__)
                                   "User defined signal 1",
                                   "User defined signal 2",
                                   "Child exited",
                                   "Power lost",
                                   "Window changed",
                                   "Urgent I/O condition",
                                   "I/O possible",
                                   "Stopped (signal)",
                                   "Stopped",
                                   "Continued",
                                   "Stopped (tty input)",
                                   "Stopped (tty output)",
                                   "Virtual timer expired",
                                   "Profiling timer expired",
                                   "CPU time limit exceeded",
                                   "File size limit exceeded",
#elif defined(__alpha__) || defined(__sparc__)
                                   "Urgent I/O condition",
                                   "Stopped (signal)",
                                   "Stopped",
                                   "Continued",
                                   "Child exited",
                                   "Stopped (tty input)",
                                   "Stopped (tty output)",
                                   "I/O possible",
                                   "CPU time limit exceeded",
                                   "File size limit exceeded",
                                   "Virtual timer expired",
                                   "Profiling timer expired",
                                   "Window changed",
                                   "Power/Resource lost",
                                   "User defined signal 1",
                                   "User defined signal 2",
#else
                                   "Stack fault",
                                   "Child exited",
                                   "Continued",
                                   "Stopped (signal)",
                                   "Stopped",
                                   "Stopped (tty input)",
                                   "Stopped (tty output)",
                                   "Urgent I/O condition",
                                   "CPU time limit exceeded",
                                   "File size limit exceeded",
                                   "Virtual timer expired",
                                   "Profiling timer expired",
                                   "Window changed",
                                   "I/O possible",
                                   "Power lost",
                                   "Bad system call",
#endif
#ifndef __hppa__
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
#endif
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
#ifdef __mips__
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
                                   "Real time signal",
#endif
                                   0};
#endif
