#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"
#include "../var.h"
#include "../../lib/path.h"
#include "../../lib/sig.h"
#include "../../lib/str.h"
#include "../../lib/uint32.h"
#include "../term.h"
#include "../prompt.h"
#include "../history.h"
#include "../job.h"
#include "../../lib/wait.h"
#include "../../lib/windoze.h"

#include <errno.h>
#include <stdlib.h>
#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

int sh_argc;
char** sh_argv;
const char* sh_name;
int sh_login = 0;
int sh_no_position = 0;

/* SIGCHLD handler -- kept to only async-signal-safe work (plain memory
 * writes: wait_nohang()/wait_nohang_untraced() + job_signal(), and a
 * write() to the job_sigfd self-pipe). It used to also call
 * term_erase()/term_restore()/prompt_show()/buffer_*() directly from
 * signal-handler context, none of which is async-signal-safe (see
 * BUGS: sh-onsig-async-unsafe, fixed as fixes/87) -- that work now
 * happens in term_read()'s select() loop (src/term/term_read.c),
 * which wakes on job_sigfd[0] instead of relying on this handler to
 * do it inline. job_clean() (src/job/job_clean.c) has grown the
 * "announce a background job that just stopped" banner this handler
 * used to print itself, for the same reason.
 * ----------------------------------------------------------------------- */
static void
sh_onsig(int signum) {
  switch(signum) {
    case SIGCHLD: {
      pid_t pid;
      int status;

      /* only ask to be woken on a stop too (WUNTRACED) in interactive
         job-control mode -- see job_wait()'s matching wait_pid()/
         wait_pid_untraced() choice for why */
      if((pid = (sh->opts.monitor ? wait_nohang_untraced : wait_nohang)(&status)) > 0)
        job_signal(pid, status);

#if !WINDOWS_NATIVE
      if(job_sigfd[1] >= 0) {
        char c = 0;
        ssize_t r;

        do
          r = write(job_sigfd[1], &c, 1);
        while(r == -1 && errno == EINTR);
      }
#endif

      break;
    }
  }
}

/* main routine
 * ----------------------------------------------------------------------- */
int
main(int argc, char** argv, char** envp) {
  int c, e, v;
  int flags;
  struct fd* fd;
  struct source src;
  char* cmds = NULL;
  struct var* envvars;
  int no_interactive = 0;

  fd_expected = STDERR_FILENO + 1;

  /* create new fds for every valid file descriptor until stderr */
  for(e = STDIN_FILENO; e <= STDERR_FILENO; e++) {
    if((flags = fdtable_check(e))) {
#ifdef HAVE_ALLOCA
      fd = fd_allocb();
      fd_push(fd, e, flags);
#else
      fd = fd_mallocb();
      fd_push(fd, e, flags | FD_FREE);
#endif
      fd_setfd(fd, e);
    } else {
      if(e < fd_expected)
        fd_expected = e;
    }
  }

  /* stat the file descriptors and then set the buffers */
  fdtable_foreach(v) {
    fd_stat(fdtable[v]);
    fd_setbuf(fdtable[v], &fdtable[v][1], FD_BUFSIZE);
  }

  /* set initial $0 */
  sh_argv0 = argv[0];

  shell_init(fd_err->w, path_basename(sh_argv0));

  /* set our basename for the \v prompt escape seq and maybe other stuff*/
  sh_name = shell_name;

  if(*sh_name == '-') {
    sh_name++;
    sh_login++;
  }

  /* import environment variables to the root vartab */
  for(c = 0; envp[c]; c++)
    ;

#ifdef HAVE_ALLOCA
  if(!(envvars = alloca(sizeof(struct var) * c)))
#endif
    envvars = alloc(sizeof(struct var) * c);

  for(c = 0; envp[c]; c++)
    var_import(envp[c], V_EXPORT, &envvars[c]);

  /* parse command line arguments */
  while((c = shell_getopt(argc, argv, "c:xe")) > 0)
    switch(c) {
      case 'c': cmds = shell_optarg; break;
      case 'x': sh->opts.xtrace = 1; break;
      case 'e': sh->opts.errexit = 1; break;

#ifdef _DEBUG
      case 'I': no_interactive = 1; break;

#endif

      default:
        sh_usage();
        sh_exit(1);
        break;
    }

    /* set up the source fd (where the shell reads from) */
#ifdef HAVE_ALLOCA
  fd = fd_alloc();
  fd_push(fd, STDSRC_FILENO, FD_READ);
#else
  fd = fd_malloc();
  fd_push(fd, STDSRC_FILENO, FD_READ | FD_FREE);
#endif

  /* if there were cmds supplied with the option
     -c then read input from this string. POSIX: "sh -c command_string
     [command_name [argument...]]" -- command_name, if present, becomes
     $0 (and is consumed here so it doesn't also leak into $1 as an
     extra positional parameter below); the remaining arguments become
     $1, $2, ... Without this, $0 stayed the shish binary's own path
     and every real argument was off by one, with command_name itself
     showing up as $1 instead of being consumed as $0
     (dash-c-argv0-not-consumed, fixes/77). */
  if(cmds) {
    fd_string(fd_src, cmds, str_len(cmds));

    if(argv[shell_optind])
      sh_argv0 = argv[shell_optind++];
  }

  /* if there is an argument we open it as input file */
  else if(argv[shell_optind]) {
    fd_mmap(fd_src, argv[shell_optind]);

    sh_argv0 = argv[shell_optind++];
  }

  /* input is read from stdin, maybe interactively */
  else if(fd_in)
    fd_dup(fd_src, STDIN_FILENO);

  if(fd_needbuf(fd_src))
    fd_setbuf(fd_src, &fd_src[1], FD_BUFSIZE);

  /* set global shell argument vector */
  sh_argv = &argv[shell_optind];
  sh_argc = argc - shell_optind;

  sh_init();

  source_push(&src);

  if((fd_src->mode & FD_CHAR) && !no_interactive && term_init(fd_src, fd_err)) {
    src.mode |= SOURCE_IACTIVE;

    sh->opts.monitor = 1;
    sh->opts.histexpand = 1;

    /* only now does fd_err->mode & FD_TERM actually reflect reality --
       term_init() above is what sets it. See job_terminal_init()'s own
       comment (BUGS: job-terminal-never-initialized) for why this
       can't just happen inside job_init() (called earlier, from
       sh_init()). */
    job_terminal_init();
  } else
    src.mode &= ~SOURCE_IACTIVE;

  sig_catch(SIGCHLD, sh_onsig);

  /*  if(fd_expected != fd_top && (flags = fdtable_check(e)))
    {
      fd = fd_allocb();
      fd_push(fd, e, flags);
      fd_setfd(fd, e);
      fdtable_track(e, FDTABLE_LAZY);
    }*/

  sh_loop();

  if(source->mode & SOURCE_IACTIVE) {
    term_restore(source->b->fd, &term_tcattr);

    history_shutdown();
  }

  sh_exit(sh->exitcode);

  return 0;
}
