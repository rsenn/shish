#include "../builtin.h"
#include "../debug.h"
#include "../tree.h"
#include "../parse.h"
#include "../source.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdtable.h"
#include "../expand.h"
#include "../../lib/sig.h"
#include "../../lib/scan.h"

typedef struct trap {
  unsigned char sig;
  union node* tree;
  struct trap* next;
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

static void
trap_install(int sig, union node* tree) {
  trap* tr;
  tr = alloc(sizeof(trap));
  tr->sig = sig;
  tr->tree = tree;
  tr->next = traps;
  tr->sh = sh;
  traps = tr;

  if(sig >= 0) {
    signal(sig, &trap_handler);
  } else if((unsigned char)sig == TRAP_DEBUG) {
    eval->debug_handler = trap_debug;
  } else if((unsigned char)sig == TRAP_RETURN) {
    struct eval* e;
    for(e = eval; e; e = e->parent)
      if(e->flags & E_FUNCTION)
        break;

    if(e)
      e->exit_handler = trap_return;
  } else if((unsigned char)sig == TRAP_EXIT) {
    struct eval* e;

    if((e = eval_find(E_ROOT)))
      e->exit_handler = trap_exit;
  }
}

static int
trap_uninstall(int sig) {
  trap** trp;
  for(trp = &traps; *trp; trp = &(*trp)->next) {
    if((*trp)->sig == sig) {
      trap* tr = *trp;
      if((char)tr->sig >= 0)
        signal(sig, SIG_DFL);
      tree_free(tr->tree);
      *trp = tr->next;
      alloc_free(tr);
      return 0;
    }
  }
  return 1;
}

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_trap(int argc, char* argv[]) {
  union node* cmds = 0;
  const char* code;
  int c, ret = 1, list = 0, print = 0;
  int signum;

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
    unsigned char num;
    for(num = TRAP_DEBUG; (name = trap_name(num)); num++) {
      if(num && (num % 5) == 0)
        buffer_putc(fd_out->w, '\n');

      buffer_putulong0(fd_out->w, num, 2);
      buffer_puts(fd_out->w, ") ");
      buffer_putspad(fd_out->w, name, 8);
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

    debug_to(buffer_2);
    debug_str("builtin_trap", code, 0, '"');
    debug_s("signum: ");
    debug_n(signum);
    debug_nl_fl();
    debug_to(&debug_buffer);

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
