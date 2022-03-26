#include <signal.h>
#include <stdio.h>

int
main() {
#ifdef SIGABRT
  printf("  %-20s /* %u */\n", "{SIGABRT, \"ABRT\"}", SIGABRT);
#endif
#ifdef SIGALRM
  printf("  %-20s /* %u */\n", "{SIGALRM, \"ALRM\"}", SIGALRM);
#endif
#ifdef SIGBUS
  printf("  %-20s /* %u */\n", "{SIGBUS, \"BUS\"}", SIGBUS);
#endif
#ifdef SIGCHLD
  printf("  %-20s /* %u */\n", "{SIGCHLD, \"CHLD\"}", SIGCHLD);
#endif
#ifdef SIGCLD
  printf("  %-20s /* %u */\n", "{SIGCLD, \"CLD\"}", SIGCLD);
#endif
#ifdef SIGCONT
  printf("  %-20s /* %u */\n", "{SIGCONT, \"CONT\"}", SIGCONT);
#endif
#ifdef SIGEMT
  printf("  %-20s /* %u */\n", "{SIGEMT, \"EMT\"}", SIGEMT);
#endif
#ifdef SIGFPE
  printf("  %-20s /* %u */\n", "{SIGFPE, \"FPE\"}", SIGFPE);
#endif
#ifdef SIGHUP
  printf("  %-20s /* %u */\n", "{SIGHUP, \"HUP\"}", SIGHUP);
#endif
#ifdef SIGILL
  printf("  %-20s /* %u */\n", "{SIGILL, \"ILL\"}", SIGILL);
#endif
#ifdef SIGINFO
  printf("  %-20s /* %u */\n", "{SIGINFO, \"INFO\"}", SIGINFO);
#endif
#ifdef SIGINT
  printf("  %-20s /* %u */\n", "{SIGINT, \"INT\"}", SIGINT);
#endif
#ifdef SIGIO
  printf("  %-20s /* %u */\n", "{SIGIO, \"IO\"}", SIGIO);
#endif
#ifdef SIGIOT
  printf("  %-20s /* %u */\n", "{SIGIOT, \"IOT\"}", SIGIOT);
#endif
#ifdef SIGKILL
  printf("  %-20s /* %u */\n", "{SIGKILL, \"KILL\"}", SIGKILL);
#endif
#ifdef SIGLOST
  printf("  %-20s /* %u */\n", "{SIGLOST, \"LOST\"}", SIGLOST);
#endif
#ifdef SIGNO
  printf("  %-20s /* %u */\n", "{SIGNO, \"NO\"}", SIGNO);
#endif
#ifdef SIGPIPE
  printf("  %-20s /* %u */\n", "{SIGPIPE, \"PIPE\"}", SIGPIPE);
#endif
#ifdef SIGPOLL
  printf("  %-20s /* %u */\n", "{SIGPOLL, \"POLL\"}", SIGPOLL);
#endif
#ifdef SIGPROF
  printf("  %-20s /* %u */\n", "{SIGPROF, \"PROF\"}", SIGPROF);
#endif
#ifdef SIGPWR
  printf("  %-20s /* %u */\n", "{SIGPWR, \"PWR\"}", SIGPWR);
#endif
#ifdef SIGQUIT
  printf("  %-20s /* %u */\n", "{SIGQUIT, \"QUIT\"}", SIGQUIT);
#endif
#ifdef SIGRESERVE
  printf("  %-20s /* %u */\n", "{SIGRESERVE, \"RESERVE\"}", SIGRESERVE);
#endif
#ifdef SIGRTMAX
  printf("  %-20s /* %u */\n", "{SIGRTMAX, \"RTMAX\"}", SIGRTMAX);
#endif
#ifdef SIGRTMIN
  printf("  %-20s /* %u */\n", "{SIGRTMIN, \"RTMIN\"}", SIGRTMIN);
#endif
#ifdef SIGSEGV
  printf("  %-20s /* %u */\n", "{SIGSEGV, \"SEGV\"}", SIGSEGV);
#endif
#ifdef SIGSTKFLT
  printf("  %-20s /* %u */\n", "{SIGSTKFLT, \"STKFLT\"}", SIGSTKFLT);
#endif
#ifdef SIGSTKSZ
  printf("  %-20s /* %u */\n", "{SIGSTKSZ, \"STKSZ\"}", SIGSTKSZ);
#endif
#ifdef SIGSTOP
  printf("  %-20s /* %u */\n", "{SIGSTOP, \"STOP\"}", SIGSTOP);
#endif
#ifdef SIGSYS
  printf("  %-20s /* %u */\n", "{SIGSYS, \"SYS\"}", SIGSYS);
#endif
#ifdef SIGTERM
  printf("  %-20s /* %u */\n", "{SIGTERM, \"TERM\"}", SIGTERM);
#endif
#ifdef SIGTRAP
  printf("  %-20s /* %u */\n", "{SIGTRAP, \"TRAP\"}", SIGTRAP);
#endif
#ifdef SIGTSTP
  printf("  %-20s /* %u */\n", "{SIGTSTP, \"TSTP\"}", SIGTSTP);
#endif
#ifdef SIGTTIN
  printf("  %-20s /* %u */\n", "{SIGTTIN, \"TTIN\"}", SIGTTIN);
#endif
#ifdef SIGTTOU
  printf("  %-20s /* %u */\n", "{SIGTTOU, \"TTOU\"}", SIGTTOU);
#endif
#ifdef SIGUNUSED
  printf("  %-20s /* %u */\n", "{SIGUNUSED, \"UNUSED\"}", SIGUNUSED);
#endif
#ifdef SIGURG
  printf("  %-20s /* %u */\n", "{SIGURG, \"URG\"}", SIGURG);
#endif
#ifdef SIGUSR1
  printf("  %-20s /* %u */\n", "{SIGUSR1, \"USR1\"}", SIGUSR1);
#endif
#ifdef SIGUSR2
  printf("  %-20s /* %u */\n", "{SIGUSR2, \"USR2\"}", SIGUSR2);
#endif
#ifdef SIGVTALRM
  printf("  %-20s /* %u */\n", "{SIGVTALRM, \"VTALRM\"}", SIGVTALRM);
#endif
#ifdef SIGWINCH
  printf("  %-20s /* %u */\n", "{SIGWINCH, \"WINCH\"}", SIGWINCH);
#endif
#ifdef SIGXCPU
  printf("  %-20s /* %u */\n", "{SIGXCPU, \"XCPU\"}", SIGXCPU);
#endif
#ifdef SIGXFSZ
  printf("  %-20s /* %u */\n", "{SIGXFSZ, \"XFSZ\"}", SIGXFSZ);
#endif
  return 0;
}
