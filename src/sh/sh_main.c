#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif
#include "../expand.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../parse.h"
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
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* envp isn't a portable third main() parameter -- POSIX only
 * guarantees it's populated when main() is actually invoked with 3
 * args, which Emscripten's callMain() never does (always calls
 * _main(argc, argv), leaving a 3rd param as 0/NULL and crashing the
 * envp[c] walk below). environ is the portable way to reach the
 * environment; every libc here (glibc/musl/dietlibc/mingw) populates
 * it regardless of how main() was called. */
#if WINDOWS_NATIVE
extern char** _environ;
#define environ _environ
#else
extern char** environ;
#endif

int sh_argc;
char** sh_argv;
const char* sh_name;
int sh_login = 0;
int sh_no_position = 0;
int sh_interactive = 0;

/* SIGCHLD handler -- kept to only async-signal-safe work: plain memory
 * writes (wait_nohang()/wait_nohang_untraced() + job_signal()) and a
 * write() to the job_sigfd self-pipe. Anything needing buffered I/O
 * (banners, terminal output) happens later in term_read()'s select()
 * loop, which wakes on job_sigfd[0] instead.
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

/* a deliberately partial expansion of $ENV's value: POSIX technically
 * requires full parameter/command/arithmetic substitution, but that's
 * too much to pull the whole parser/expander pipeline in for just a
 * startup-file pathname. Tilde expansion plus simple "$NAME"/"${NAME}"
 * substitution (falling back to "" if unset) covers the common cases
 * ("$HOME/.shishrc", "~/.shishrc").
 * ----------------------------------------------------------------------- */
static void
sh_expand_simple(const char* in, stralloc* out) {
  size_t i = 0, len = str_len(in);
  stralloc home;
  size_t prefixlen;

  stralloc_init(&home);

  if(expand_tilde_lookup(in, len, 0, &home, &prefixlen)) {
    stralloc_cat(out, &home);
    i = prefixlen;
  }

  stralloc_free(&home);

  for(; i < len; i++) {
    if(in[i] == '$' && i + 1 < len) {
      int braced = in[i + 1] == '{';
      size_t start = i + 1 + (braced ? 1 : 0);
      size_t end = start;

      while(end < len && parse_isname(in[end], end - start))
        end++;

      if(end > start) {
        stralloc name;
        const char* value;
        size_t vlen;

        stralloc_init(&name);
        stralloc_catb(&name, in + start, end - start);
        stralloc_nul(&name);

        value = var_vdefault(name.s, "", &vlen);
        stralloc_catb(out, value, vlen);
        stralloc_free(&name);

        i = (braced && end < len && in[end] == '}') ? end : end - 1;
        continue;
      }
    }

    stralloc_catc(out, in[i]);
  }
}

/* main routine
 * ----------------------------------------------------------------------- */
int
main(int argc, char** argv) {
  int c, e, v;
  int flags;
  struct fd* fd;
  struct source src;
  char* cmds = NULL;
  struct var* envvars;
  int no_interactive = 0;
  int force_interactive = 0;
  int read_stdin = 0;

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
  for(c = 0; environ[c]; c++)
    ;

#ifdef HAVE_ALLOCA
  if(!(envvars = alloca(sizeof(struct var) * c)))
#endif
    envvars = alloc(sizeof(struct var) * c);

  for(c = 0; environ[c]; c++)
    var_import(environ[c], V_EXPORT, &envvars[c]);

  /* $SHELL should point at this shell, not whatever shell was running
     before shish exec'd -- override whatever var_import() above just
     pulled in from the environment with our own invocation path. */
  var_setv("SHELL", argv[0], str_len(argv[0]), V_EXPORT);

  /* POSIX: privileged mode turns on automatically, before any option
     is looked at, whenever real and effective uid/gid differ; an
     explicit "-p"/"+p" below still overrides it. */
#if !WINDOWS_NATIVE
  if(geteuid() != getuid() || getegid() != getgid())
    sh->opts.privileged = 1;
#endif

  /* parse command line arguments. Every letter "set" supports
   * (src/builtin/builtin_set.c's set_apply()/set_longopts) works
   * identically as a startup flag, including "-o name"/"+o name" by
   * long name. "-c"/"-i"/"-l"/"-s" are startup-only.
   *
   * A *local* struct optstate, not the process-global shell_getopt(),
   * is used for the same reason as builtin_set.c's identical loop:
   * shell_optind is never reset between unrelated builtins' own
   * shell_getopt() calls, so state would otherwise leak into a later
   * builtin sharing the same global. */
  {
    struct optstate opt = {"+-", 0, 0, 0, 0, 0};

    while((c = shell_getopt_r(&opt, argc, argv, "+c:isloaefhmnpuxBCH")) > 0) {
      int on = opt.prefix == '-';

      switch(c) {
        case 'c': cmds = opt.arg; break;
        case 'i': force_interactive = on; break;
        case 's': read_stdin = 1; break;
        case 'l': sh_login = on; break;

        case 'o': {
          char* name;

          /* shell_getopt_r() only advances past the *whole* current
             argv element for an option that takes an argument via
             ":" -- "o" has none (its own argument is read by hand
             here, same as builtin_set.c's identical handling), so
             opt.ind is still pointing at "-o"/"+o" itself here. */
          opt.ind++;
          opt.ofs = 0;
          name = argv[opt.ind];

          if(name) {
            size_t i;
            int letter = 0;

            for(i = 0; i < set_longopts_n; i++) {
              if(str_equal(set_longopts[i].name, name)) {
                letter = set_longopts[i].letter;
                break;
              }
            }

            if(!letter) {
              sh_usage();
              sh_exit(1);
            }

            set_apply(&sh->opts, letter, on);
            opt.ind++;
          }

          break;
        }

#ifdef _DEBUG
        case 'I': no_interactive = 1; break;

#endif

        default:
          if(!set_apply(&sh->opts, c, on)) {
            sh_usage();
            sh_exit(1);
          }

          break;
      }
    }

    /* the rest of this function still reads shell_optind (the
       process-global one) below, same as it always has -- only the
       parsing loop itself needed to move off the shared state. */
    shell_optind = opt.ind;
  }

  /* set up the source fd (where the shell reads from) */
#ifdef HAVE_ALLOCA
  fd = fd_alloc();
  fd_push(fd, STDSRC_FILENO, FD_READ);
#else
  fd = fd_malloc();
  fd_push(fd, STDSRC_FILENO, FD_READ | FD_FREE);
#endif

  /* if -c supplied a command string, read input from it. POSIX: "sh -c
     command_string [command_name [argument...]]" -- command_name, if
     present, becomes $0 (consumed here so it doesn't leak into $1);
     the remaining arguments become $1, $2, ... */
  if(cmds) {
    fd_string(fd_src, cmds, str_len(cmds));

    if(argv[shell_optind])
      sh_argv0 = argv[shell_optind++];
  }

  /* if there is an argument we open it as input file -- unless "-s"
     said to read from stdin regardless, in which case every remaining
     argument (this one included) is left alone to become a plain
     positional parameter instead of being consumed as a script $0. */
  else if(!read_stdin && argv[shell_optind]) {
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

  {
    /* "-i" forces interactive behavior on even without a real
       terminal, but term_init() still only succeeds against an actual
       char-device fd, so job_terminal_init() below stays gated on
       that succeeding regardless. */
    int have_term = (fd_src->mode & FD_CHAR) && !no_interactive && term_init(fd_src, fd_err);

    if(have_term || (force_interactive && !no_interactive)) {
      src.mode |= SOURCE_IACTIVE;
      sh_interactive = 1;

#if !WINDOWS_NATIVE
      /* monitor mode drives setpgid()/tcsetpgrp() (job_fork.c,
       * job_wait.c, job_foreground.c) -- none of which have a
       * Windows target (no controlling-terminal/foreground-process-
       * group concept, see doc/building.md). Leaving it off keeps
       * every job synchronous/foreground-only there instead of
       * running job-control bookkeeping for primitives that never
       * actually execute. */
      sh->opts.monitor = 1;
#endif
      sh->opts.histexpand = 1;

      /* only now does fd_err->mode & FD_TERM reflect reality --
         term_init() above is what sets it. */
      if(have_term)
        job_terminal_init();
    } else
      src.mode &= ~SOURCE_IACTIVE;
  }

  /* dash/ash: a login shell unconditionally reads /etc/profile, then
     $HOME/.profile -- silently skipping either that doesn't exist,
     regardless of interactivity or privileged mode (neither file's
     path is attacker-influenced the way $ENV's value is, so there's
     no analogous privilege-escalation reason to gate this on -p).
     Runs before the $ENV read below since a .profile commonly sets
     ENV itself (dash(1)'s own documented idiom: "ENV=$HOME/.shinit;
     export ENV" inside .profile), and that must take effect for the
     ENV read that follows. */
  if(sh_login) {
    const char* home = var_vdefault("HOME", NULL, NULL);

    sh_source("/etc/profile");

    if(home && *home) {
      stralloc homepath;

      stralloc_init(&homepath);
      stralloc_cats(&homepath, home);
      stralloc_cats(&homepath, "/.profile");
      stralloc_nul(&homepath);
      sh_source(homepath.s);
      stralloc_free(&homepath);
    }
  }

  /* POSIX: an interactive, non-privileged shell sources the file
     named by $ENV (after expansion) once at startup, if it names an
     existing, readable file -- silently skipped otherwise (a missing
     $ENV file is not an error), matching real shells. */
  if((src.mode & SOURCE_IACTIVE) && !sh->opts.privileged) {
    const char* envval = var_vdefault("ENV", NULL, NULL);

    if(envval && *envval) {
      stralloc envpath;

      stralloc_init(&envpath);
      sh_expand_simple(envval, &envpath);
      stralloc_nul(&envpath);
      sh_source(envpath.s);
      stralloc_free(&envpath);
    }
  }

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
