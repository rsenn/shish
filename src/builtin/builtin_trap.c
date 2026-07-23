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
#include "../../lib/sig.h"
#include "../../lib/scan.h"
#include <assert.h>

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

static void
trap_handler(int sig) {
  trap* tr;
  struct eval e;

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

  eval_push(&e, 0);
  eval_tree(&e, tr->tree, 0);
  eval_pop(&e);
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
      if((char)t->sig > 0)
        sig_pop(sig);

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
       per-signal stack, installing trap_handler with SA_MASKALL so
       another signal can't re-enter it while a trap action is
       already running -- trap_uninstall()'s matching sig_pop() above
       restores exactly what was there before, not just SIG_DFL. */
    sig_push(sig, &trap_handler);

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
      if((char)t->sig > 0)
        sig_pop(t->sig);

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

      if(cmds)
        trap_install(signum, cmds);
      else
        ret = trap_uninstall(signum);
    }
  }

  return ret;
}
