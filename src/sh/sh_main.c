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

/* a deliberately partial expansion of $ENV's value: POSIX technically
 * requires full parameter/command/arithmetic substitution, but that's
 * far more than a startup-file *pathname* is worth pulling the whole
 * parser/expander pipeline in for. Tilde expansion (reusing
 * expand_tilde_lookup(), see src/expand/expand_tilde.c) plus simple
 * "$NAME"/"${NAME}" parameter substitution (falling back to "" for an
 * unset/malformed one, same as real expansion would with -u off)
 * covers the overwhelming majority of real ENV values in the wild
 * ("$HOME/.shishrc", "~/.shishrc") -- anything fancier is a known,
 * accepted gap.
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
main(int argc, char** argv, char** envp) {
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
  for(c = 0; envp[c]; c++)
    ;

#ifdef HAVE_ALLOCA
  if(!(envvars = alloca(sizeof(struct var) * c)))
#endif
    envvars = alloc(sizeof(struct var) * c);

  for(c = 0; envp[c]; c++)
    var_import(envp[c], V_EXPORT, &envvars[c]);

  /* POSIX: privileged mode turns on automatically, before any
     command-line option is even looked at, whenever the real and
     effective uid or gid differ (e.g. a setuid/setgid shish binary,
     or invoked via one) -- an explicit "-p"/"+p" below still takes
     the final word either way, same as every other option's default
     being overridable on the command line. */
#if !WINDOWS_NATIVE
  if(geteuid() != getuid() || getegid() != getgid())
    sh->opts.privileged = 1;
#endif

  /* parse command line arguments -- every letter "set" supports
     (src/builtin/builtin_set.c's set_apply()/set_longopts, shared
     from there rather than duplicated here) works identically as a
     startup flag, including "-o name"/"+o name" by long name; "-c"
     belongs here specifically (command_string only makes sense at
     startup); "-i"/"-s" are startup-only, meaningless as "set"
     options.

     A *local* struct optstate, not the process-global shell_getopt()/
     shell_opt -- exactly like builtin_set.c's own identical loop, and
     for the identical reason: shell_optind is never reset to 0
     between unrelated builtins' own shell_getopt() calls (confirmed
     -- nothing in this codebase does that), so whatever "+-" vs. "-"
     prefixes a leading '+' in *this* optstring left in the *shared*
     global state would otherwise silently leak into every later
     builtin built on that same global (e.g. builtin_chmod.c's own
     "v"), making "chmod +x file" misparse "+x" as an (invalid)
     option instead of chmod's own plain mode-string operand. */
  {
    struct optstate opt = {"+-", 0, 0, 0, 0, 0};

    while((c = shell_getopt_r(&opt, argc, argv, "+c:isoaefhmnpuxBCH")) > 0) {
      int on = opt.prefix == '-';

      switch(c) {
        case 'c': cmds = opt.arg; break;
        case 'i': force_interactive = 1; break;
        case 's': read_stdin = 1; break;

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
    /* "-i" forces interactive behavior (prompting, job control,
       history) on even without a real terminal to back it -- but
       term_init() itself (the line-editing/job-control-signal setup)
       still only succeeds against an actual char-device fd, so
       job_terminal_init() below is gated on that succeeding
       regardless of how we got here. */
    int have_term =
        (fd_src->mode & FD_CHAR) && !no_interactive && term_init(fd_src, fd_err);

    if(have_term || (force_interactive && !no_interactive)) {
      src.mode |= SOURCE_IACTIVE;

      sh->opts.monitor = 1;
      sh->opts.histexpand = 1;

      /* only now does fd_err->mode & FD_TERM actually reflect reality
         -- term_init() above is what sets it. See
         job_terminal_init()'s own comment (BUGS:
         job-terminal-never-initialized) for why this can't just
         happen inside job_init() (called earlier, from sh_init()). */
      if(have_term)
        job_terminal_init();
    } else
      src.mode &= ~SOURCE_IACTIVE;
  }

  /* POSIX: an interactive, non-privileged shell sources the file
     named by $ENV (after expansion) once at startup, if it names an
     existing, readable file -- silently skipped otherwise (a missing
     $ENV file is not an error), matching real shells. */
  if((src.mode & SOURCE_IACTIVE) && !sh->opts.privileged) {
    const char* envval = var_vdefault("ENV", NULL, NULL);

    if(envval && *envval) {
      stralloc envpath;
      struct fd envfd;
      struct source envsrc;

      stralloc_init(&envpath);
      sh_expand_simple(envval, &envpath);
      stralloc_nul(&envpath);

      fd_push(&envfd, STDSRC_FILENO, FD_READ);
      source_push(&envsrc);
      envsrc.fd = &envfd;

      if(!fd_mmap(&envfd, envpath.s))
        sh_loop();

      source_popfd(&envfd);
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
