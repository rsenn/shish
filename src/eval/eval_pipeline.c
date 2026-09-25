#include "../../lib/alloc.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../fd.h"
#include "../sh.h"
#include "../builtin.h"
#include "../eval.h"
#include "../expand.h"
#include "../exec.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../job.h"
#include "../tree.h"
#include "../var.h"
#include "../debug.h"
#include "../../lib/byte.h"
#include "../../lib/str.h"
#include "../../lib/wait.h"
#include "../../lib/windoze.h"
#include "builtin_config.h"
#if !WINDOWS_NATIVE
#include <sys/wait.h>
#include <unistd.h>
#endif

#if BUILTIN_TRAP
void* trap_snapshot_save(void);
void trap_snapshot_restore(void*);
#endif

/* filter-chain scanner (TODO.md Goal 13): finds a trailing run of
 * adjacent pure-builtin pipeline stages that can hand off through an
 * in-process buffer (struct filter_ops, src/builtin_filter.h) instead
 * of a real fork()+pipe() pair. Scoped deliberately narrow -- false
 * negatives (falling back to the always-correct fork()+pipe() path)
 * are always safe, so every check below is conservative on purpose.
 * ----------------------------------------------------------------------- */

extern union node* functions; /* exec_search.c; see term_complete.c etc. for the same extern */

/* true iff arg (an N_ARG word) is a single, bare, unquoted literal --
 * no parameter/command/arithmetic expansion, no glob, no quoting --
 * so reading it needs no expansion machinery, hence no question of
 * *where* (parent vs. the stage's own subshell environment) any
 * expansion side effect would run: there isn't one. *s/*n point at
 * the literal's raw (not NUL-terminated) bytes. */
static int
pipeline_word_literal(union node* arg, const char** s, size_t* n) {
  union node* w;

  if(!arg || arg->id != N_ARG)
    return 0;

  w = arg->narg.list;

  if(!w || w->next || w->id != N_ARGSTR || w->nargstr.flag != S_UNQUOTED)
    return 0;

  *s = w->nargstr.stra.s;
  *n = w->nargstr.stra.len;
  return 1;
}

/* resolves a literal command name to a filter-capable builtin, using
 * exec_search()'s own precedence for the one case that matters here:
 * a same-named function shadows the builtin at real exec time, so a
 * shadowed name must not chain (it wouldn't be this builtin that
 * actually runs). Special builtins and external commands can't be
 * filter-capable (only ordinary B_DEFAULT builtins register a
 * filter), so unlike exec_search() this never needs a PATH lookup. */
static struct builtin_cmd*
pipeline_filter_builtin(const char* name) {
  struct builtin_cmd* b = builtin_search((char*)name, B_DEFAULT);
  struct nfunc* fn;

  if(!b || !b->filter || !b->filter->ops)
    return NULL;

  for(fn = functions ? &functions->nfunc : NULL; fn; fn = fn->next)
    if(!str_diff(name, fn->name))
      return NULL;

  return b;
}

/* pipeline_filter_prepare: node chains iff it's a plain simple command
 * (no local assignments, no redirections of its own -- both would
 * need real fd/scope machinery this path skips) whose entire word
 * list is bare literals (pipeline_word_literal(), above) naming a
 * filter-capable builtin. On success returns that builtin and fills
 * *argv_out/*argc_out with freshly copied, NUL-terminated argv
 * strings the caller owns (pipeline_filter_argv_free()'s job).
 * ----------------------------------------------------------------------- */
static struct builtin_cmd*
pipeline_filter_prepare(union node* node, char*** argv_out, int* argc_out) {
  union node* a;
  int argc = 0, i;
  char** argv;
  const char* s0;
  size_t n0;
  char name[64];
  struct builtin_cmd* b;

  if(!node || node->id != N_SIMPLECMD || node->ncmd.vars || node->ncmd.rdir || !node->ncmd.args)
    return NULL;

  for(a = node->ncmd.args; a; a = a->next) {
    const char* s;
    size_t n;

    if(!pipeline_word_literal(a, &s, &n))
      return NULL;

    argc++;
  }

  pipeline_word_literal(node->ncmd.args, &s0, &n0); /* argv[0]; already validated above */

  if(n0 >= sizeof(name))
    return NULL;

  byte_copy(name, n0, s0);
  name[n0] = 0;

  if(!(b = pipeline_filter_builtin(name)))
    return NULL;

  argv = alloc((size_t)(argc + 1) * sizeof(char*));

  if(!argv)
    return NULL;

  for(a = node->ncmd.args, i = 0; a; a = a->next, i++) {
    const char* s;
    size_t n;

    pipeline_word_literal(a, &s, &n);
    argv[i] = alloc(n + 1);

    if(!argv[i]) {
      while(i > 0)
        alloc_free(argv[--i]);

      alloc_free(argv);
      return NULL;
    }

    byte_copy(argv[i], n, s);
    argv[i][n] = 0;
  }

  argv[argc] = NULL;
  *argv_out = argv;
  *argc_out = argc;
  return b;
}

static void
pipeline_filter_argv_free(char** argv) {
  int i;

  if(!argv)
    return;

  for(i = 0; argv[i]; i++)
    alloc_free(argv[i]);

  alloc_free(argv);
}

#if !defined(HAVE_FORK)
/* evaluate a pipeline without fork() (3.9.2) -- see
 * notes/pipeline-sequential.md for the full design. Each stage runs
 * to completion, fully in-process, before the next one starts: no
 * pipe(2), no concurrency, no streaming between stages, just the same
 * non-forking "run this subtree as its own isolated subshell
 * environment" machinery eval_subshell.c already uses for "(...)" --
 * POSIX already specifies every pipeline component as running in its
 * own subshell environment (2.9.2), so this isn't an approximation of
 * that isolation, it's the same isolation. A non-last stage's stdout
 * is captured into an in-memory buffer via fd_subst() (the same
 * mechanism "$(...)" uses); the next stage reads that buffer as its
 * stdin via fd_here() (the same mechanism here-documents use).
 * Inherits the same known gap eval_subshell.c already documents for
 * persistent redirections across a non-forking subshell boundary --
 * see TODO.md, Goal 4.
 *
 * The *last* stage is the one deliberate exception: it runs directly
 * against the caller's own environment instead, matching zsh/ksh's
 * "lastpipe" behavior (see eval_pipeline()'s matching comment below).
 * ----------------------------------------------------------------------- */
static int
eval_pipeline_sequential(struct eval* e, struct npipe* npipe) {
  union node* node;
  char* prev_s = NULL;
  size_t prev_len = 0;
  int have_prev = 0;
  int last_ret = 0;

  if(npipe->bgnd) {
    /* nothing to background a pipeline *onto* when nothing can
       fork() -- running it synchronously anyway would silently hide
       that "&" did nothing, so this errors loudly instead. Loud over
       silent, matching the entire reason this path exists (see
       eval-pipeline-silent-on-fork-failure in BUGS). */
    sh->exitcode = sh_error("background pipelines are not supported without a working fork()");
    return sh->exitcode;
  }

  for(node = npipe->cmds; node; node = node->next) {
    int is_last = (node->next == NULL);
    stralloc captured;
    struct fdstack io;
    struct fd_state fdst;
    struct fd out_fd, in_fd;
    int ret;

    stralloc_init(&captured);

    fdstack_push(&io);
    fd_state_save(&fdst);

    if(!is_last)
      fd_subst(fd_push(&out_fd, STDOUT_FILENO, FD_WRITE), &captured);

    if(have_prev) {
      stralloc from_prev;

      from_prev.s = prev_s;
      from_prev.len = prev_len;
      fd_here(fd_push(&in_fd, STDIN_FILENO, FD_READ), &from_prev);
    }

    if(is_last) {
      /* zsh/ksh run a pipeline's *last* command in the current shell
         instead of isolating it too, so e.g. "cmd | read x" sets $x
         here instead of in a throwaway subshell -- matches the
         lastpipe branch in eval_pipeline() below, and needs none of
         that branch's job-control caveat: this path never forks
         anything at all, so there is no process group for this stage
         to be left out of. eval_tree() already updates sh->exitcode
         as a side effect; any exit/jump this stage triggers correctly
         unwinds through the caller's own setjmp frame instead of a
         local one, exactly as it would for a plain top-level command. */
      eval_tree(e, node, 0);
      ret = sh->exitcode;
    } else {
      struct vartab vars;
      struct env she;
      struct func_snapshot funcs;
#if BUILTIN_TRAP
      void* traps_snap;
#endif
      struct eval en;
      int jmpret;

      vartab_push(&vars, 0);
      sh_push(&she);
      exec_functions_save(&funcs);
#if BUILTIN_TRAP
      traps_snap = trap_snapshot_save();
#endif

      eval_push(&en, E_ROOT);

      /* set up a long jump so we can exit this stage and end up just
         after the setjmp call, which will return nonzero in this case */
      en.jump = 1;
      jmpret = setjmp(en.jumpbuf);

      if(jmpret) {
        en.exitcode = (jmpret >> 1);
      } else {
        /* neither E_LIST nor E_EXIT: node->next here is the *next
           pipeline stage*, not more of this one, so this must evaluate
           node alone -- E_LIST would make eval_tree() walk straight
           into the next stage as if it were part of this one's own
           list. E_EXIT means "safe to execve() this disposable
           process" (see eval_cmdlist.c); there is no disposable
           process here, only the one real, ongoing shell. */
        eval_tree(&en, node, 0);

        if(en.destructor)
          en.exitcode = en.destructor(en.exitcode);
      }

      ret = eval_pop(&en);

#if BUILTIN_TRAP
      trap_snapshot_restore(traps_snap);
#endif
      exec_functions_restore(&funcs);
      sh_pop(&she);
      vartab_pop(&vars);

      /* a real-signal-triggered "exit" (sh_async_exit) needs to keep
         propagating past this stage, same as eval_subshell.c's
         identical block -- this call never returns when it fires */
      if((jmpret & 1) && sh_async_exit)
        sh_exit(ret);
    }

    if(have_prev)
      fd_pop(&in_fd); /* frees prev_s, via fd_here()'s deinit */

    if(!is_last)
      fd_pop(&out_fd); /* does not free captured.s -- fd_subst() sets
                           no deinit; ownership passes to the next
                           stage's fd_here() call below instead */

    fdstack_pop(&io);
    fd_state_restore(&fdst);

    last_ret = ret;

    if(!is_last) {
      prev_s = captured.s;
      prev_len = captured.len;
      have_prev = 1;
    }
  }

  sh->exitcode = last_ret;
  return sh->exitcode;
}
#endif /* !defined(HAVE_FORK) */

/* evaluate a pipeline (3.9.2)
 * ----------------------------------------------------------------------- */
int
eval_pipeline(struct eval* e, struct npipe* npipe) {
#if !defined(HAVE_FORK)
  return eval_pipeline_sequential(e, npipe);
#else
  union node* node;
  struct fdstack st;
  struct fd* pipes = 0;
  unsigned int n;
  int pid = 0, prevfd = -1, status = -1;
  struct job* job;

  /* filter chaining (TODO.md Goal 13) -- see the comment where chain_b
     is populated, below. chain_ctx/chain_ops are only ever non-NULL
     once open() has actually committed, for the one loop iteration
     that immediately follows. */
  struct builtin_cmd* chain_b = NULL;
  char** chain_argv = NULL;
  int chain_argc = 0;
  void* chain_ctx = NULL;
  const struct filter_ops* chain_ops = NULL;

  /* zsh/ksh run a foreground pipeline's *last* command in the current
     shell instead of forking it too, so e.g. "cmd | read x" sets $x
     here instead of in a throwaway subshell -- bash needs "shopt -s
     lastpipe" to opt in; this matches zsh/ksh's unconditional default
     instead. Restricted to job-control-inactive pipelines (matching
     bash's own restriction on lastpipe) because job_fork() puts a
     pipeline's forked members in their own process group and hands
     them the terminal when monitor mode is on (see job_fork.c) --
     running the last member unforked here would leave it outside
     that group while it's still the terminal's foreground group. */
  int lastpipe = !npipe->bgnd && !sh->opts.monitor;

  if((job = job_new(npipe->ncmd - (lastpipe ? 1 : 0)))) {
    job->bgnd = npipe->bgnd;
  } else {
    buffer_puts(fd_err->w, "no job control");
    buffer_putnlflush(fd_err->w);
  }

  /* filter chaining (TODO.md Goal 13), scoped for now to exactly one
     link: an exactly-2-stage pipeline's *first* stage, feeding
     directly into the true last stage (already unforked via lastpipe
     above, so there's no fd-lifetime hazard in overwriting its stdin
     below). chain_b/argv/argc are the *candidate*; a candidate can
     still decline at open() time for a reason the static AST check
     below can't see (grep -c/-q, a bad pattern), in which case this
     one stage just falls through to job_fork() exactly as if no
     candidate had ever been found.

     Deliberately NOT extended to a 3+-stage pipeline's second-to-last
     stage yet, even though pipeline_filter_prepare() itself doesn't
     care where in the pipeline its node sits: that stage's own
     upstream ("in" below) would be a real, already-forked
     predecessor's pipe, wired into an alloca()'d struct fd that is
     only valid for *this* loop iteration (see fd_alloc()/HAVE_ALLOCA
     above) -- but the chain's ctx stores that upstream buffer* for
     the *true last* stage's iteration to read from, several loop
     iterations later, after "in"'s stack space has already been
     reused. Confirmed by testing: "grep a | sed ..." (2 stages, this
     stage's upstream is the persistent fd_in->r) works; "cat | grep a
     | sed ..." (grep's upstream would be a transient real "in")
     silently produced no output. Lifting this restriction needs the
     real fd dup()'d into a heap-owned buffer before this iteration's
     "in" goes out of scope, not just a wider node-position check. */
  if(lastpipe && npipe->ncmd == 2) {
    chain_b = pipeline_filter_prepare(npipe->cmds, &chain_argv, &chain_argc);
  }

  fdstack_push(&st);

  for(node = npipe->cmds; node; node = node->next) {
    struct fd *in = 0, *out = 0;
    char inbuf[FD_BUFSIZE];
    int is_last = (node->next == NULL);

    /* if there was a previous command we read input from pipe */
    if(prevfd >= 0) {

#ifdef HAVE_ALLOCA
      in = fd_alloc();
      fd_push(in, STDIN_FILENO, FD_READ | FD_PIPE);
#else
      in = fd_malloc();
      fd_push(in, STDIN_FILENO, FD_READ | FD_PIPE | FD_FREE);
#endif
      fd_setfd(in, prevfd);

      /* fd_init() (via fd_push()) leaves ->r with a NULL, zero-length
         buffer -- fine for a *forked external* program (it never
         reads through this struct at all, just inherits the raw pipe
         fd via dup2()), but a builtin runs in-process and reads
         through fd_in->r directly. read(fd, NULL, 0) is well-defined
         to return 0 immediately, which buffer_get_until() (and
         everything built on it) can't tell apart from real EOF --
         "cmd | builtin_that_reads_stdin" silently produced no output
         at all, for every such builtin, confirmed with "echo hi | cat"
         (redir-pipeline-builtin-stdin-unbuffered, fixes/90). */
      if(fd_needbuf(in))
        fd_setbuf(in, inbuf, sizeof(inbuf));
    }

    /* if it isn't the last command we have to create a pipe
       to pass output to the next command */
    if(node->next /* || (fd_out->mode & FD_SUBST) == FD_SUBST */) {

#ifdef HAVE_ALLOCA
      out = fd_alloc();
      fd_push(out, STDOUT_FILENO, FD_WRITE | FD_PIPE);
#else
      in = fd_malloc();
      fd_push(out, STDOUT_FILENO, FD_WRITE | FD_PIPE | FD_FREE);
#endif

      if((prevfd = fd_pipe(out)) == -1) {
        /* prevfd is already -1 here; close(-1) is a no-op that only
           risks clobbering errno (with EBADF) before it's reported */
        sh_error_errno("pipe creation failed");
      }
    }

    /* fdstack_npipes()/fdstack_pipe() wire a real pipe for a
       command-substitution target found in the fdstack (fd_subst()
       only sets up an in-process stralloc sink, nothing a forked
       child can write into -- and job_fork() always forks, even for
       a builtin). This only makes sense for the *last* pipeline
       member: it's the only one whose stdout the substitution target
       actually cares about, and calling it for an earlier member
       would hijack that member's stdout away from the inter-stage
       pipe ("out" above) that's supposed to feed the next member's
       stdin instead. exec_program.c uses this same pair for the
       (pipeline-free) command-substitution case. Skipped when
       "is_last && lastpipe": that member never forks below, so its
       stdout already reaches the substitution's fd_subst() sink
       directly, the same way any other in-process command's would --
       no real pipe needs to bridge a fork that isn't happening. */
    if(!node->next && !(is_last && lastpipe) && (n = fdstack_npipes(FD_HERE | FD_SUBST))) {
      pipes = alloc(FDSTACK_ALLOC_SIZE(n));
      fdstack_pipe(n, pipes);
    }

    if(is_last && lastpipe) {
      if(chain_ctx) {
        /* the chained stage committed last iteration: repurpose this
           (already fd_push()'d) stdin to read from it instead of the
           real pipe fd_setfd() wired above -- fd_close() first, or
           fd_filter()'s own buffer_init() would silently orphan that
           real fd (nothing else still points at it to close later). */
        fd_close(in);
        fd_filter(in, chain_ops, chain_ctx);
      }

      /* run directly in the current shell instead of forking -- see
         the "lastpipe" comment above. eval_simple_command() already
         updates sh->exitcode as a side effect, so nothing further is
         needed to make "$?" reflect this stage; E_EXIT is also wrong
         here for the same reason it's wrong in
         eval_pipeline_sequential() -- there is no disposable forked
         process to execve() into, only the one real, ongoing shell. */
      eval_tree(e, node, 0);
    } else if(chain_b && node->next && node->next->next == NULL) {
      /* the chain candidate found before the loop -- try it for real,
         now that this stage's actual upstream (in ? in->r : fd_in->r,
         exactly what job_fork() would otherwise have handed a forked
         child) is available. shell_optind/shell_optofs must be reset
         exactly as exec_command.c resets them before calling any
         builtin's fn() -- open() runs shell_getopt() over chain_argv
         too, and a stale cursor left over from an earlier command
         indexes past this small argv array instead of parsing it. */
      shell_optind = 1;
      shell_optofs = 0;
      chain_ops = chain_b->filter->ops;
      chain_ctx = chain_ops->open(chain_argc, chain_argv, in ? in->r : fd_in->r);

      if(!chain_ctx) {
        /* declined for a reason the static check couldn't see (grep
           -c/-q, a bad pattern, ...): in/out this iteration are the
           same real, untouched fds job_fork() would use anyway, so
           this is exactly as if no candidate had been found. */
        chain_ops = NULL;

        if(!(pid = job_fork(job, node, npipe->bgnd))) {
          exit(eval_tree(e, node, E_EXIT));
        }
      } else {
        /* committed: no fork, no output for this stage -- out (this
           stage's real, now-unused outgoing pipe) is simply closed as
           dead plumbing by the existing fd_pop(out) below, same as
           any other unused pipe; the job this pipeline no longer
           forks for must shrink to match, or job_wait()'s loop would
           wait on a proc slot job_fork() never filled. */
        if(job)
          job->nproc--;
      }
    } else if(!(pid = job_fork(job, node, npipe->bgnd))) {
      /* no job control for commands inside pipe */
      /*e->mode &= E_JCTL;*/

      /* exit after evaluating this subtree */
      exit(eval_tree(e, node, E_EXIT));
    } else {
#ifdef DEBUG_OUTPUT
      debug_ulong("forked", pid, 0);
#endif
    }

    if(!node->next && pipes) {
      unsigned int i;

      /* the pipe write-end(s) fdstack_pipe() created above are only
         needed by the child we just forked into -- close our own
         (parent-side) copies before draining, or fdstack_data()'s
         read() would never see EOF (our own open copy would keep the
         pipe writable forever). Matches exec_program.c's
         fdstack_pop(&io) (which is fdstack_pop(&st) below, but that
         has to wait until every pipeline member has run) followed by
         fdstack_data() -- the same shared drain used there, so a
         command substitution nested inside this pipeline's own
         members gets read back correctly too, not just this one. */
      for(i = 0; i < n; i++)
        fd_pop(&pipes[i]);

      fdstack_data();
    }

    if(out)
      fd_pop(out);

    if(in)
      fd_pop(in);
  }

  fdstack_pop(&st);

  if(!npipe->bgnd) {
    job_wait(job, 0, &status);
  } else {
    /* backgrounding a pipeline ("cmd1 | cmd2 &") succeeds as soon as
       it's launched -- status is never touched by job_wait() (it's
       not called for a bgnd pipeline), so leaving it at its initial
       -1 would report a bogus 255 as "$?" once that's wired up below */
    status = 0;
  }

  if(pipes)
    alloc_free(pipes);

  if(job)
    job_free(job);

  if(chain_b)
    pipeline_filter_argv_free(chain_argv);

  /* eval_simple_command() updates sh->exitcode directly (not just its
     return value) so "$?" sees a command's status immediately, even
     from a later command on the very same line -- e->exitcode only
     gets synced back to sh->exitcode once the *whole* line's
     eval_tree() call returns (see eval_pop()/sh_loop.c), which is too
     late for "cmd1 | cmd2; echo $?" sitting on one line together.
     Without this, "$?" after a pipeline kept reporting whatever it
     was *before* the pipeline ran. Skipped for "lastpipe": "status"
     there is job_wait()'s view of the *other*, forked members --
     the true last member already set sh->exitcode itself, directly,
     by running unforked above; POSIX's pipeline exit status is that
     last member's, not any earlier one's. */
  if(!lastpipe)
    sh->exitcode = WAIT_STATUS(status);

  return sh->exitcode;
#endif /* !defined(HAVE_FORK) */
}
