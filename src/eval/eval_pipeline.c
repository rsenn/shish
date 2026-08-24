#include "../../lib/alloc.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../exec.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../job.h"
#include "../tree.h"
#include "../var.h"
#include "../debug.h"
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
      /* run directly in the current shell instead of forking -- see
         the "lastpipe" comment above. eval_simple_command() already
         updates sh->exitcode as a side effect, so nothing further is
         needed to make "$?" reflect this stage; E_EXIT is also wrong
         here for the same reason it's wrong in
         eval_pipeline_sequential() -- there is no disposable forked
         process to execve() into, only the one real, ongoing shell. */
      eval_tree(e, node, 0);
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
