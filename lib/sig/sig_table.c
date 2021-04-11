#include "../windoze.h"
#include "../sig_internal.h"

#include <signal.h>

sigtable_t const sigtable[] = {{0, "EXIT"},
#if !WINDOWS_NATIVE
                               {SIGHUP, "HUP"},       /* 1 */
                               {SIGINT, "INT"},       /* 2 */
                               {SIGQUIT, "QUIT"},     /* 3 */
                               {SIGILL, "ILL"},       /* 4 */
                               {SIGTRAP, "TRAP"},     /* 5 */
                               {SIGABRT, "ABRT"},     /* 6 */
                               {SIGBUS, "BUS"},       /* 7 */
                               {SIGFPE, "FPE"},       /* 8 */
                               {SIGKILL, "KILL"},     /* 9 */
                               {SIGUSR1, "USR1"},     /* 10 */
                               {SIGSEGV, "SEGV"},     /* 11 */
                               {SIGUSR2, "USR2"},     /* 12 */
                               {SIGPIPE, "PIPE"},     /* 13 */
                               {SIGALRM, "ALRM"},     /* 14 */
                               {SIGTERM, "TERM"},     /* 15 */
                               {SIGSTKFLT, "STKFLT"}, /* 16 */
                               {SIGCHLD, "CHLD"},     /* 17 */
                               {SIGCONT, "CONT"},     /* 18 */
                               {SIGSTOP, "STOP"},     /* 19 */
                               {SIGTSTP, "TSTP"},     /* 20 */
                               {SIGTTIN, "TTIN"},     /* 21 */
                               {SIGTTOU, "TTOU"},     /* 22 */
                               {SIGURG, "URG"},       /* 23 */
                               {SIGXCPU, "XCPU"},     /* 24 */
                               {SIGXFSZ, "XFSZ"},     /* 25 */
                               {SIGVTALRM, "VTALRM"}, /* 26 */
                               {SIGPROF, "PROF"},     /* 27 */
                               {SIGWINCH, "WINCH"},   /* 28 */
                               {SIGIO, "IO"},         /* 29 */
                               {SIGPWR, "PWR"},       /* 30 */
                               {SIGSYS, "SYS"},       /* 31 */
#endif
                               {0, 0}};
