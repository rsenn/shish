/* pty-run.c: run a command under a real pty-backed controlling
 * terminal, for the tests/posix .tst cases gated on "../checkfg"
 * (job control, sigttin/sigttou/sigtstp, testtty-p, wait-p) that
 * ctest currently skips outright for lack of one.
 *
 * Usage: pty-run [-t seconds] -- command [args...]
 *
 * The child becomes a session leader with the new pty's slave side
 * as its controlling terminal (fd 0 and 1; fd 2 is left alone, so a
 * caller redirecting stdout/stderr separately -- as
 * tests/posix/run-test.sh does -- still gets them apart). Everything
 * this process itself reads from its own stdin is relayed verbatim
 * to the pty master, and everything the pty produces is relayed to
 * this process's own stdout -- so no special "send a signal" API is
 * needed: a literal ^Z/^C/^\ byte already flowing through as part of
 * a test's normal stdin reaches the slave's line discipline exactly
 * as if a user had typed it, and the kernel raises the matching
 * SIGTSTP/SIGINT/SIGQUIT for the pty's foreground process group on
 * its own. SIGTTIN/SIGTTOU need no simulation at all: any background
 * process group that reads/writes the terminal triggers them from
 * the kernel side the moment there's a real session+pty to enforce
 * that against.
 * ----------------------------------------------------------------------- */
#define _XOPEN_SOURCE 600
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static pid_t child_pid = -1;

static void
on_alarm(int sig) {
  (void)sig;

  if(child_pid > 0)
    kill(-child_pid, SIGKILL);
}

static int
relay(int master) {
  char buf[4096];
  int in_open = 1;

  for(;;) {
    fd_set rfds;
    int maxfd = master;
    ssize_t n;

    FD_ZERO(&rfds);
    FD_SET(master, &rfds);

    if(in_open) {
      FD_SET(STDIN_FILENO, &rfds);

      if(STDIN_FILENO > maxfd)
        maxfd = STDIN_FILENO;
    }

    if(select(maxfd + 1, &rfds, 0, 0, 0) < 0) {
      if(errno == EINTR)
        continue;

      return -1;
    }

    if(in_open && FD_ISSET(STDIN_FILENO, &rfds)) {
      n = read(STDIN_FILENO, buf, sizeof(buf));

      if(n > 0)
        (void)!write(master, buf, (size_t)n);
      else
        in_open = 0; /* EOF/error on our own stdin: stop forwarding it,
                        keep draining the child's output below */
    }

    if(FD_ISSET(master, &rfds)) {
      n = read(master, buf, sizeof(buf));

      if(n > 0) {
        (void)!write(STDOUT_FILENO, buf, (size_t)n);
      } else {
        /* EOF: the slave side has no more openers (child exited and
           closed its copy) -- normal end of relay */
        return 0;
      }
    }
  }
}

int
main(int argc, char* argv[]) {
  int master, slave, status, timeout = 0, i = 1;
  char* slavename;
  pid_t w;

  if(argc > 2 && !strcmp(argv[1], "-t")) {
    timeout = atoi(argv[2]);
    i = 3;
  }

  if(i < argc && !strcmp(argv[i], "--"))
    i++;

  if(i >= argc) {
    fprintf(stderr, "usage: pty-run [-t seconds] -- command [args...]\n");
    return 2;
  }

  if((master = posix_openpt(O_RDWR | O_NOCTTY)) < 0) {
    perror("posix_openpt");
    return 1;
  }

  if(grantpt(master) < 0 || unlockpt(master) < 0 || !(slavename = ptsname(master))) {
    perror("grantpt/unlockpt/ptsname");
    return 1;
  }

  /* a plausible interactive default; test scripts that care set their
     own size explicitly */
  {
    struct winsize ws = {24, 80, 0, 0};
    ioctl(master, TIOCSWINSZ, &ws);
  }

  if((child_pid = fork()) < 0) {
    perror("fork");
    return 1;
  }

  if(child_pid == 0) {
    close(master);

    if(setsid() < 0)
      _exit(126);

    if((slave = open(slavename, O_RDWR)) < 0)
      _exit(126);

#ifdef TIOCSCTTY
    if(ioctl(slave, TIOCSCTTY, 0) < 0)
      _exit(126);
#endif

    if(dup2(slave, STDIN_FILENO) < 0 || dup2(slave, STDOUT_FILENO) < 0)
      _exit(126);

    if(slave > STDERR_FILENO)
      close(slave);

    execvp(argv[i], &argv[i]);
    _exit(127);
  }

  /* parent: never touch the slave by name again -- only the child
     needs it, and opening it here too would make us a second session
     member with our own reference to the same controlling terminal */
  if(timeout > 0) {
    signal(SIGALRM, on_alarm);
    alarm((unsigned)timeout);
  }

  relay(master);
  close(master);

  w = waitpid(child_pid, &status, 0);

  if(w < 0)
    return 1;

  if(WIFEXITED(status))
    return WEXITSTATUS(status);

  if(WIFSIGNALED(status))
    return 128 + WTERMSIG(status);

  return 1;
}
