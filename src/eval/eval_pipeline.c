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

/* true iff arg (an N_ARG word) is a single bare word -- no parameter/
 * command/arithmetic expansion (those are separate N_ARGSTR-sibling or
 * different-id nodes, never this one) -- whose bytes are already
 * final, needing no expansion machinery to read: unquoted, or quoted
 * (single or double) with nothing in it that needed the parser's own
 * glob-protection escaping (parse_isesc(), src/parse.h). That escaping
 * inserts a literal '\\' ahead of a protected byte regardless of quote
 * style, indistinguishable here from a "real" backslash the word
 * actually contains -- telling those apart needs exactly the
 * expand_unescape() pass this whole path exists to avoid, so any
 * backslash at all declines instead. *s / *n point at the literal's raw
 * (not NUL-terminated) bytes. */
static int
pipeline_word_literal(union node* arg, const char** s, size_t* n) {
  union node* w;

  if(!arg || arg->id != N_ARG)
    return 0;

  w = arg->narg.list;

  if(!w || w->next || w->id != N_ARGSTR)
    return 0;

  switch(w->nargstr.flag) {
    case S_UNQUOTED:
    case S_SQUOTED:
    case S_DQUOTED: break;
    default: return 0;
  }

  if(byte_chr(w->nargstr.stra.s, w->nargstr.stra.len, '\\') < w->nargstr.stra.len)
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
 * *argv_out / *argc_out with freshly copied, NUL-terminated argv
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

/* pipeline_filter_prepare_chain: pipeline_filter_prepare() run over
 * every non-last stage of the pipeline at once -- the static half of
 * "N adjacent builtin filters chain straight into the true last
 * stage" (TODO.md Goal 13). All ncmd-1 non-last stages have to
 * qualify or none do: one stage that doesn't falls the *whole*
 * pipeline back to today's job_fork()/fd_pipe() path, unchanged --
 * there's no partial chain here, only "all" or "nothing chains".
 * Returns the stage count (ncmd-1) on success, filling *b_out /
 * *argv_out / *argc_out (each an ncmd-1-element array the caller
 * owns, parallel to npipe->cmds); returns 0 on failure, having 
 * freed anything it already allocated. */
static int
pipeline_filter_prepare_chain(struct npipe* npipe,
                              struct builtin_cmd*** b_out,
                              char**** argv_out,
                              int** argc_out) {
  int n = (int)npipe->ncmd - 1;
  struct builtin_cmd** b;
  char*** argv;
  int* argc;
  union node* node;
  int i;

  if(n <= 0)
    return 0;

  b = alloc((size_t)n * sizeof(*b));
  argv = alloc((size_t)n * sizeof(*argv));
  argc = alloc((size_t)n * sizeof(*argc));

  if(!b || !argv || !argc) {
    alloc_free(b);
    alloc_free(argv);
    alloc_free(argc);
    return 0;
  }

  for(node = npipe->cmds, i = 0; node->next; node = node->next, i++) {
    if(!(b[i] = pipeline_filter_prepare(node, &argv[i], &argc[i]))) {
      while(i > 0) {
        i--;
        pipeline_filter_argv_free(argv[i]);
      }

      alloc_free(b);
      alloc_free(argv);
      alloc_free(argc);
      return 0;
    }
  }

  *b_out = b;
  *argv_out = argv;
  *argc_out = argc;
  return n;
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
     is populated, below. chain_b/argv/argc (chain_n entries) are the
     static candidates; chain_ctx/chain_ops/chain_committed reflect how
     many of them actually opened, which can be fewer (0, on decline)
     but never more. chain_link holds the chain_committed-1 in-process
     buffers that feed one opened stage's output into the next one's
     open() as "upstream" -- the true last stage never gets one of
     these, it reads the final opened stage directly (see the
     "is_last && lastpipe" branch below). */
  struct builtin_cmd** chain_b = NULL;
  char*** chain_argv = NULL;
  int* chain_argc = NULL;
  int chain_n = 0;
  void** chain_ctx = NULL;
  const struct filter_ops** chain_ops = NULL;
  buffer* chain_link = NULL;
  int chain_committed = 0;
  int stage;

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

  /* filter chaining (TODO.md Goal 13), scoped to a pipeline's *entire*
     non-last prefix, feeding directly into the true last stage
     (already unforked via lastpipe above, so there's no fd-lifetime
     hazard in overwriting its stdin below). chain_b/argv/argc are the
     *candidates* (pipeline_filter_prepare_chain()'s static AST check,
     above); all of them open() successfully or none of them do -- the
     loop below never partially commits a chain, so there is no stage
     left needing a heap-owned upstream buffer it doesn't already have:
     the only stage that ever reads a real fd is stage 0 (fd_in->r,
     persistent already), every later chained stage's upstream is one
     of the in-process buffers this same pre-pass builds
     (chain_link[], below) -- unlike a chain rooted anywhere else in
     the pipeline, this one never needs a forked predecessor's transient
     per-iteration pipe fd to outlive that iteration.

     A candidate can still decline at open() time for a reason the
     static check can't see (grep -c/-q, a bad pattern, ...); since
     open() itself has no side effects before it commits (see
     builtin_filter.h), declining stage i just means every stage before
     it gets rolled back too (their .close() runs right here) and the
     whole pipeline runs exactly as if no candidate had ever been
     found. */
  if(lastpipe)
    chain_n = pipeline_filter_prepare_chain(npipe, &chain_b, &chain_argv, &chain_argc);

  if(chain_n > 0) {
    buffer* upstream = fd_in->r;

    chain_ctx = alloc((size_t)chain_n * sizeof(*chain_ctx));
    chain_ops = alloc((size_t)chain_n * sizeof(*chain_ops));

    if(chain_n > 1)
      chain_link = alloc((size_t)(chain_n - 1) * sizeof(*chain_link));

    for(stage = 0; stage < chain_n; stage++) {
      /* exec_command.c resets these the same way before calling any
         builtin's fn() -- open() runs shell_getopt() over chain_argv
         too, and a stale cursor left over from an earlier stage (or an
         earlier command entirely) indexes past this stage's own small
         argv array instead of parsing it. */
      shell_optind = 1;
      shell_optofs = 0;
      chain_ops[stage] = chain_b[stage]->filter->ops;
      chain_ctx[stage] = chain_ops[stage]->open(chain_argc[stage], chain_argv[stage], upstream);

      if(!chain_ctx[stage]) {
        int j;

        /* all-or-nothing (see the comment above this loop): every
           earlier stage in this attempt gets rolled back too, not
           just left "committed" -- job_new() below sizes its proc
           table from chain_committed, and a stage that's actually
           going to job_fork() below needs a slot in it. */
        for(j = 0; j < stage; j++)
          chain_link[j].deinit(&chain_link[j]);

        chain_committed = 0;
        break;
      }

      chain_committed++;

      if(stage < chain_n - 1) {
        buffer_filter_init(&chain_link[stage], chain_ops[stage], chain_ctx[stage]);
        upstream = &chain_link[stage];
      }
    }
  }

  if((job = job_new(npipe->ncmd - (lastpipe ? 1 : 0) - chain_committed))) {
    job->bgnd = npipe->bgnd;
  } else {
    buffer_puts(fd_err->w, "no job control");
    buffer_putnlflush(fd_err->w);
  }

  fdstack_push(&st);

  for(node = npipe->cmds, stage = 0; node; node = node->next, stage++) {
    struct fd *in = 0, *out = 0;
    char inbuf[FD_BUFSIZE];
    int is_last = (node->next == NULL);
    /* this stage is one of the chain_committed opened before the loop
       -- it never forks, never gets a real pipe, and (below) never
       does anything at all in this loop: it already ran, on demand,
       as whatever later stage pulled it through chain_link[]/the true
       last stage's fd_filter() wiring. */
    int chained = stage < chain_committed;

    /* if there was a previous command we read input from pipe -- or,
       if this is the true last (lastpipe) stage right after a fully
       committed chain, from that chain instead (prevfd is untouched
       at -1 for the whole chain: none of its stages ever got a real
       pipe), in which case "in" still has to exist so the "is_last &&
       lastpipe" branch below has something to fd_close()+fd_filter(). */
    if(prevfd >= 0 || (is_last && lastpipe && chain_committed > 0)) {

#ifdef HAVE_ALLOCA
      in = fd_alloc();
      fd_push(in, STDIN_FILENO, FD_READ | (prevfd >= 0 ? FD_PIPE : 0));
#else
      in = fd_malloc();
      fd_push(in, STDIN_FILENO, FD_READ | (prevfd >= 0 ? FD_PIPE : 0) | FD_FREE);
#endif
      if(prevfd >= 0) {
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
    }

    /* if it isn't the last command we have to create a pipe to pass
       output to the next command -- unless this stage is chained: it
       never runs for real, so it never has an "out" to feed anything
       through a real pipe either. */
    if(node->next && !chained /* || (fd_out->mode & FD_SUBST) == FD_SUBST */) {

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
      if(chain_committed > 0) {
        /* the whole chain ahead of this stage already opened, before
           the loop: repurpose this (already fd_push()'d) stdin to
           read from its last link instead of a real pipe -- fd_close()
           first, or fd_filter()'s own buffer_init() would silently
           orphan the fd_push() above (nothing else still points at it
           to close later). */
        fd_close(in);
        fd_filter(in, chain_ops[chain_committed - 1], chain_ctx[chain_committed - 1]);
      }

      /* run directly in the current shell instead of forking -- see
         the "lastpipe" comment above. eval_simple_command() already
         updates sh->exitcode as a side effect, so nothing further is
         needed to make "$?" reflect this stage; E_EXIT is also wrong
         here for the same reason it's wrong in
         eval_pipeline_sequential() -- there is no disposable forked
         process to execve() into, only the one real, ongoing shell. */
      eval_tree(e, node, 0);
    } else if(chained) {
      /* already opened in the pre-pass before this loop, and reads on
         demand through chain_link[]/the true last stage's fd_filter()
         wiring above -- nothing left to do for it here. job_new()
         was already sized without this stage, so job_wait() below
         doesn't wait on a proc slot job_fork() never filled. */
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

  if(chain_b) {
    int j;

    /* chain_link[0..chain_committed-2]: the last committed stage's ctx
       already closed above, via fd_pop(in)'s fd_filter_deinit() --
       every earlier one only ever got wrapped in one of these, so this
       is the one place left that closes it (buffer_filter_init() wired
       ops->close(ctx) into (b)->deinit itself). Nothing to do here for
       a declined chain (chain_committed == 0): the pre-pass already
       rolled every opened stage back to this same state before the
       loop ever ran. */
    for(j = 0; j < chain_committed - 1; j++)
      chain_link[j].deinit(&chain_link[j]);

    alloc_free(chain_link);
    alloc_free(chain_ctx);
    alloc_free(chain_ops);

    for(j = 0; j < chain_n; j++)
      pipeline_filter_argv_free(chain_argv[j]);

    alloc_free(chain_argv);
    alloc_free(chain_argc);
    alloc_free(chain_b);
  }

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
