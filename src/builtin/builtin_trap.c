#include "../builtin.h"
#include "../debug.h"
#include "../tree.h"
#include "../parse.h"
#include "../source.h"
#include "../sh.h"
#include "../eval.h"
#include "../exec.h"
#include "../fdtable.h"
#include "../expand.h"
#include "../job.h"
#include "../../lib/sig.h"
#include "../../lib/scan.h"
#include <assert.h>
#include <signal.h>
#include <unistd.h>

typedef struct trap_s {
  unsigned char sig;
  union node* tree;
  struct trap_s* next;
  struct env* sh;
} trap;

trap* traps = 0;
// static trap** trap_link = &traps;

enum { TRAP_DEBUG = 254, TRAP_RETURN = 255, TRAP_EXIT = 0 };

static const char*
trap_name(int sig) {
  switch((unsigned char)sig) {
    case TRAP_RETURN: return "RETURN";
    case TRAP_DEBUG: return "DEBUG";
    case TRAP_EXIT: return "EXIT";
    default: return sig_name(sig);
  }

  return 0;
}

static int
trap_byname(const char* name) {
  if(parse_isdigit(name[0])) {
    int sig;
    scan_int(name, &sig);
    return sig;
  }

  if(!str_case_diff(name, "RETURN"))
    return TRAP_RETURN;

  if(!str_case_diff(name, "DEBUG"))
    return TRAP_DEBUG;

  if(!str_case_diff(name, "EXIT"))
    return TRAP_EXIT;

  return sig_byname(name);
}

static trap*
trap_find(int sig) {
  trap* tr;

  for(tr = traps; tr; tr = tr->next)
    if(tr->sig == (unsigned char)sig)
      return tr;

  return 0;
}

static void
trap_print(trap* tr) {
  buffer_puts(fd_out->w, "trap '");
  tree_print(tr->tree, fd_out->w);
  buffer_putm_internal(fd_out->w, "' ", trap_name(tr->sig), 0);
  buffer_putnlflush(fd_out->w);
}

/* set by trap_relay() (the real OS signal handler, installed via
 * sig_push() below) for whichever real signal just fired; drained by
 * trap_run_pending(), which does the actual, unsafe-to-do-in-a-
 * handler work (see its own comment). volatile sig_atomic_t is the
 * one type POSIX guarantees is safe to read/write from both a signal
 * handler and ordinary code without a data race. Indexed by raw
 * signal number -- the pseudo-signals TRAP_EXIT (0)/TRAP_DEBUG (254)/
 * TRAP_RETURN (255) are never real OS signals, so trap_relay() (only
 * ever invoked by the kernel for a real one) never touches those
 * slots. */
static volatile sig_atomic_t trap_pending[256];
volatile sig_atomic_t trap_signaled = 0;

static void
trap_handler(int sig) {
  trap* tr;
  struct eval e;
  int was_async;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_BUILTIN)
  debug_to(buffer_2);
  debug_s("trap handler ");
  debug_n(sig);
  debug_nl_fl();
  debug_to(&debug_buffer);
#endif

  if(!(tr = trap_find(sig))) {
    sh_exit(1);
    return;
  }

  /* a real-signal trap (sig > 0; this function also handles the
     EXIT/DEBUG/RETURN pseudo-signals via trap_debug()/trap_return()/
     trap_exit() below, which call it directly rather than through
     trap_run_pending()) fires asynchronously -- interrupting whatever
     the script happened to be doing, possibly deep inside an
     in-process subshell. An "exit" in its body must terminate the
     whole process regardless of that; see sh_async_exit's own comment
     (sh.h) and eval_subshell.c's use of it. The EXIT/DEBUG/RETURN
     pseudo-traps are always invoked synchronously instead -- a
     natural, in-order consequence of normal control flow (a function
     returning, a subshell ending) -- where "exit" inside them
     legitimately stays scoped to whatever subshell it's naturally
     nested in, same as an ordinary synchronous "exit" would be, so
     they must NOT set this. */
  was_async = sh_async_exit;

  if((char)sig > 0)
    sh_async_exit = 1;

  eval_push(&e, 0);
  eval_tree(&e, tr->tree, 0);
  eval_pop(&e);

  /* only reached if the trap body didn't itself exit -- an exit that
     did already terminated the process by the time anything gets
     back here (see eval_subshell.c), so there's nothing stale left to
     restore in that case. */
  sh_async_exit = was_async;
}

/* the real OS-level signal handler installed by trap_install() for a
 * real signal (via sig_push()) -- kept to the bare minimum of async-
 * signal-safe work (a plain memory write, same as sh_onsig()'s own
 * SIGCHLD handler; see BUGS: sh-onsig-async-unsafe, fixes/87) instead
 * of running the trap body here directly. The trap body used to run
 * straight from this handler, via eval_tree() -- allocation-heavy,
 * definitely not async-signal-safe, and, worse, whatever syscall this
 * interrupted (typically job_wait()'s blocking wait_pid()) would just
 * transparently resume once this handler returned (SA_RESTART), so a
 * trap that itself called "exit" never got a chance to run until
 * ordinary control flow happened to reach a dispatch point on its
 * own -- observed as a real repro: SIGINT during a "gcc" invocation
 * killed gcc (its own, unrelated default disposition) but shish
 * itself just continued to the next command as if nothing happened.
 * sig_push() now installs with SA_NORESTART for exactly this reason:
 * the interrupted syscall needs to actually return (EINTR) so
 * trap_run_pending() gets a chance to run promptly (see
 * waitpid_nointr()'s matching change). The byte written to
 * job_sigfd[1] wakes term_read()'s select() loop the same way
 * sh_onsig() already does for SIGCHLD. */
static void
trap_relay(int sig) {
  trap_pending[(unsigned char)sig] = 1;
  trap_signaled = 1;

  if(job_sigfd[1] >= 0) {
    char b = 0;
    write(job_sigfd[1], &b, 1);
  }
}

/* runs every trap whose signal fired since the last call, from
 * ordinary (non-signal-handler) context -- see trap_relay() above.
 * Called from sh_loop()'s main loop, term_read()'s select() wakeup,
 * and job_wait()'s own retry loop, so a trap fires promptly even
 * while blocked waiting on a long-running external command, not just
 * between top-level statements.
 * ----------------------------------------------------------------------- */
void
trap_run_pending(void) {
  int sig;

  if(!trap_signaled)
    return;

  trap_signaled = 0;

  for(sig = 1; sig < 254; sig++) {
    if(trap_pending[sig]) {
      trap_pending[sig] = 0;
      trap_handler(sig);
    }
  }
}

void
trap_debug(union node* tree) {
  trap* tr;

  if((tr = trap_find(TRAP_DEBUG))) {
    struct env sh;
    stralloc stra;
    char* args[2] = {0, 0};

    stralloc_init(&stra);
    tree_cat(tree, &stra);
    args[0] = stra.s;
    sh_push(&sh);
    sh_setargs(args, 0);
    trap_handler(TRAP_DEBUG);
    sh_pop(&sh);
  }
}

int
trap_return(int result) {
  trap* tr;

  if((tr = trap_find(TRAP_RETURN))) {
    struct env sh;
    char* args[2] = {alloc(FMT_ULONG), 0};

    args[0][fmt_ulong(args[0], result)] = '\0';
    sh_push(&sh);
    sh_setargs(args, 0);
    trap_handler(TRAP_RETURN);
    sh_pop(&sh);
  }

  return result;
}

int
trap_exit(int exitcode) {
  trap* tr;

  if((tr = trap_find(TRAP_EXIT))) {
    struct env sh;
    char* args[2] = {alloc(FMT_ULONG), 0};

    args[0][fmt_ulong(args[0], exitcode)] = '\0';
    sh_push(&sh);
    sh_setargs(args, 0);
    trap_handler(TRAP_EXIT);
    sh_pop(&sh);
  }

  return exitcode;
}

/* remove any existing trap for exactly this signal/pseudo-signal --
 * used both by the "trap - SIG" builtin syntax and, before installing
 * a new one, by trap_install() itself. Every real-signal trap has a
 * matching sig_push() (below); popping it here is what keeps a
 * "trap CMD SIG" run twice for the same SIG from leaking the old tree
 * and list node forever (the previous, un-deduplicated code did, and
 * every trap_install() call added another list entry that trap_find()
 * could never see again, since it always stops at the first --
 * newest -- match) and, more importantly, from exhausting
 * sig_push()'s small fixed-size per-signal stack (SIGSTACKSIZE, 16)
 * after a handful of retraps.
 * ----------------------------------------------------------------------- */
static int
trap_uninstall(int sig) {
  trap **ptr, *t;

  for(ptr = &traps; (t = *ptr); ptr = &(*ptr)->next) {
    if(t->sig == (unsigned char)sig) {
      if((char)t->sig > 0) {
        sig_pop(sig);

        /* discard any not-yet-dispatched occurrence of this signal:
           trap_relay() may have already set trap_pending[sig] (the
           signal was delivered) without trap_run_pending() having
           had a chance to drain it yet (e.g. it fired while still
           inside a subshell that's now exiting and uninstalling its
           own trap for this signal right here). Left set, the next
           trap_run_pending() call would dispatch it against whatever
           trap (if any) is *now* installed instead -- if that's
           none, trap_handler() hits its "no trap found" fallback and
           calls sh_exit(1), silently killing the whole process for
           what looks like a completely untrapped signal. Confirmed
           via a real repro: "( trap 'echo x' TERM; kill -TERM $$ )"
           with nothing after the kill -- the subshell's own
           trap_snapshot_restore() (which also hits this same path)
           uninstalls the TERM trap before the pending signal from
           the kill is ever drained, and the shell exits(1) instead
           of continuing past the subshell. */
        trap_pending[(unsigned char)sig] = 0;
      }

      *ptr = t->next;

      /* while a subshell is evaluating, the parent shell's
         trap_snapshot_save() may still reference this node (see
         eval_subshell.c) -- leak it for now (orphaned but safe, same
         tradeoff eval_function.c already makes for redefined
         functions via the same exec_subshell_depth counter);
         trap_snapshot_restore() frees anything left over once the
         subshell that's actually responsible for it exits. */
      if(!exec_subshell_depth) {
        tree_free(t->tree);
        alloc_free(t);
      }

      return 0;
    }
  }

#if defined(DEBUG_OUTPUT) && defined(DEBUG_BUILTIN)
  debug_to(buffer_2);
  debug_s("trap_uninstall ");
  debug_n(sig);
  debug_nl_fl();
  debug_to(&debug_buffer);
#endif

  return 1;
}

static void
trap_install(int sig, union node* tree) {
  struct eval* e;
  trap* tr;

  /* replace, don't stack, any trap already installed for this exact
     signal -- see trap_uninstall()'s comment */
  trap_uninstall(sig);

  tr = alloc(sizeof(trap));

  assert(tr);

  tr->sig = sig;
  tr->tree = tree;
  tr->next = traps;
  tr->sh = sh;
  traps = tr;

  if((char)sig > 0) {

    /* sig_push() (lib/sig) saves whatever disposition the signal
       already had (SIG_DFL, for a signal never trapped before) on a
       per-signal stack, installing trap_relay (not trap_handler
       itself -- see its own comment for why) with SA_MASKALL so
       another signal can't re-enter it while it runs, and
       SA_NORESTART so a blocking syscall this interrupts actually
       returns instead of transparently resuming -- trap_uninstall()'s
       matching sig_pop() above restores exactly what was there
       before, not just SIG_DFL. */
    sig_push(sig, &trap_relay);

  } else if((unsigned char)sig == TRAP_DEBUG) {
    e = eval_find(E_ROOT);

    assert(e);

    e->debug = trap_debug;

  } else if(sig == TRAP_EXIT || (unsigned char)sig == TRAP_RETURN) {

    if((e = eval_find(sig ? E_FUNCTION : E_ROOT)))
      e->destructor = sig ? trap_return : trap_exit;

  } else {
    assert(0);
  }
}

/* snapshot/restore the global trap list around a subshell (see
 * eval_subshell.c), the same way exec_functions_save/restore isolates
 * function definitions -- without this, any "trap CMD SIG" run inside
 * "( ... )" (including its own implicit EXIT trap) stayed installed
 * after the subshell "returned" (eval_subshell doesn't fork; it's the
 * same process, so nothing else would ever remove it), and fired
 * later on the *parent's* next exit with the parent's own fdstack/
 * stdout in effect (subshell-exit-trap-output-misdirected). The saved
 * array is NULL-terminated rather than paired with a count, since a
 * live trap node pointer is never itself NULL.
 * ----------------------------------------------------------------------- */
void*
trap_snapshot_save(void) {
  trap* p;
  trap** nodes;
  size_t n = 0, i;

  for(p = traps; p; p = p->next)
    n++;

  if(!n)
    return NULL;

  nodes = alloc((n + 1) * sizeof(trap*));

  for(p = traps, i = 0; p; p = p->next, i++)
    nodes[i] = p;

  nodes[n] = NULL;

  return nodes;
}

void
trap_snapshot_restore(void* handle) {
  trap** nodes = handle;
  trap* t = traps;
  size_t n = 0, k;

  if(nodes)
    while(nodes[n])
      n++;

  /* undo anything installed fresh during the subshell: a node not
     present in the saved array didn't exist when the subshell
     started, so free it and, for a real signal, sig_pop() to revert
     the OS-level handler trap_install() gave it. A node that DOES
     appear in the saved array survived because trap_uninstall()'s
     exec_subshell_depth guard skipped freeing it even if the
     subshell removed or replaced it -- relinking the saved array
     below restores the parent's exact list regardless of whatever
     surgery happened to it in the meantime. */
  while(t) {
    trap* next = t->next;
    int keep = 0;

    for(k = 0; k < n; k++) {
      if(nodes[k] == t) {
        keep = 1;
        break;
      }
    }

    if(!keep) {
      if((char)t->sig > 0) {
        sig_pop(t->sig);

        /* same reasoning as trap_uninstall()'s matching fix: a
           subshell's own trap for this signal is about to stop
           existing, so any not-yet-dispatched occurrence of it must
           not be left to fire later against whatever (if anything)
           is installed once we're back in the parent. */
        trap_pending[(unsigned char)t->sig] = 0;
      }

      tree_free(t->tree);
      alloc_free(t);
    }

    t = next;
  }

  for(k = 0; k + 1 < n; k++)
    nodes[k]->next = nodes[k + 1];

  if(n) {
    nodes[n - 1]->next = NULL;
    traps = nodes[0];
  } else {
    traps = NULL;
  }

  if(nodes)
    alloc_free(nodes);
}

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_trap[] = "    Run a command when the shell receives a signal.\n"
                         "\n"
                         "    -l              list signal names and numbers\n"
                         "    -p              print traps currently set for signal_spec (or all)\n"
                         "    arg             command to run when signal_spec is received;\n"
                         "                    '-' restores the signal's default action\n"
                         "    signal_spec     signal name/number, or EXIT/DEBUG/RETURN\n";

int
builtin_trap(int argc, char* argv[]) {
  union node* cmds = 0;
  const char* code;
  int c, ret = 1, list = 0, print = 0, signum;

  /* check options, -l for list, -p for output */
  while((c = shell_getopt(argc, argv, "lp")) > 0) {
    switch(c) {
      case 'l': list = 1; break;
      case 'p': print = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(argc < 2)
    print = 1;

  if(list) {
    const char* name;
    unsigned char num, count = 0;

    for(num = TRAP_DEBUG; (name = trap_name(num)); num++, count++) {
      if(count && (count % 5) == 0)
        buffer_putc(fd_out->w, '\n');

      buffer_putulong0(fd_out->w, num, 3);
      buffer_puts(fd_out->w, ") ");
      buffer_putspad(fd_out->w, name, 6);
    }

    buffer_putnlflush(fd_out->w);
    return 0;
  }

  if(print) {
    trap* tr;

    if(shell_optind == argc) {
      const char* name;
      unsigned char num;

      for(num = TRAP_DEBUG; (name = trap_name(num)); num++) {
        if((tr = trap_find(num)))
          trap_print(tr);
      }

      return 0;
    }

    for(; shell_optind < argc; shell_optind++) {
      if((signum = trap_byname(argv[shell_optind])) == -1) {
        builtin_errmsg(argv, argv[shell_optind], "no such signal");
        return 1;
      }

      if((tr = trap_find(signum)))
        trap_print(tr);
    }

    return 0;
  }

  if(argc < 3) {
    builtin_errmsg(argv, "usage", "trap [-lp] [[arg] signal_spec ...]");
    return 2;
  }

  code = argv[shell_optind++];

  if(str_diff(code, "-")) {
    struct fd fd;
    struct source src;
    struct parser p;

    /* create a new i/o context and initialize a parser */
    source_buffer(&src, &fd, code, str_len(code));
    parse_init(&p, P_DEFAULT);

    /* parse the string as a compound list */
    cmds = parse_compound_list(&p, 0);

    source_popfd(&fd);
  }

  for(; argv[shell_optind]; shell_optind++) {
    if((signum = trap_byname(argv[shell_optind])) == -1) {
      builtin_errmsg(argv, argv[shell_optind], "no such signal");

      if(cmds)
        tree_free(cmds);

      return 1;
    }

#if defined(DEBUG_OUTPUT) && defined(DEBUG_BUILTIN)
    debug_to(buffer_2);
    debug_s("builtin_trap ");
    debug_n(signum);
    debug_c(' ');
    debug_str("code", code, 0, 0);
    debug_nl_fl();
    debug_to(&debug_buffer);
#endif

    if(signum != 1) {
      ret = 0;

      /* "trap CODE SIG1 SIG2 ..." parses CODE only once (cmds, above)
         but installs it against every listed signal -- each one needs
         its own independent tree, not a shared pointer: trap_install()
         (via trap_uninstall()) tree_free()s whatever tree the signal
         it's replacing/removing owned, and two trap entries sharing
         the same tree pointer means the second free operates on
         already-freed memory. Confirmed via a real autoconf-generated
         script: libtool's generated cleanup traps ("trap 'rm -f
         "libtoolT"; exit 1' 1 2 15", later removed with "trap - 1 2
         15") corrupted the heap this way on the second and third
         signal every single time, eventually crashing on some later,
         unrelated allocation ("malloc_consolidate(): unaligned
         fastbin chunk detected") once the corruption was finally
         noticed (trap-install-shares-one-tree-across-multiple-
         signals, BUGS). */
      if(cmds)
        trap_install(signum, tree_copy(cmds));
      else
        ret = trap_uninstall(signum);
    }
  }

  if(cmds)
    tree_free(cmds);

  return ret;
}
