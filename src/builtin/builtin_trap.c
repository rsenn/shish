#include "../builtin.h"
#include "../debug.h"
#include "../tree.h"
#include "../parse.h"
#include "../source.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdtable.h"
#include "../../lib/sig.h"

typedef struct trap {
  int signal;
  union node* tree;
  struct trap* next;
  struct env* sh;
} trap;

trap* traps = 0;
// static trap** trap_link = &traps;

static trap*
trap_find(int sig) {
  trap* tr;

  for(tr = traps; tr; tr = tr->next)
    if(tr->signal == sig)
      return tr;

  return 0;
}

static void
trap_handler(int sig) {
  trap* tr;

  if(!(tr = trap_find(sig))) {
    sh_exit(1);
    return;
  }

  eval_tree(sh->eval, tr->tree, 0);
}

static void
trap_install(int sig, union node* tree) {
  trap* tr;

  tr = alloc(sizeof(trap));

  tr->signal = sig;
  tr->tree = tree;
  tr->next = traps;
  tr->sh = sh;

  traps = tr;

  sig_catch(sig, &trap_handler);
}

static int
trap_uninstall(int sig) {
  trap** tptr;

  for(tptr = &traps; *tptr; tptr = &(*tptr)->next) {
    if((*tptr)->signal == sig) {
      trap* tr = *tptr;
      tree_free(tr->tree);
      *tptr = tr->next;
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
  int c, ret = 1, signum = -1, list = 0, print = 0;

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
    for(signum = 0; (name = sig_name(signum)); signum++) {
      if(signum && (signum % 5) == 0)
        buffer_putc(fd_out->w, '\n');

      buffer_putulong0(fd_out->w, signum, 2);
      buffer_puts(fd_out->w, ") ");
      buffer_putspad(fd_out->w, name, 8);
    }

    buffer_putnlflush(fd_out->w);
    return 0;
  }

  if(print) {

    if(shell_optind == argc) {
      const char* name;
      for(signum = 0; (name = sig_name(signum)); signum++) {
        trap* tr;
        if((tr = trap_find(signum))) {
          buffer_puts(fd_out->w, "trap '");
          tree_print(tr->tree, fd_out->w);
          buffer_putm_internal(fd_out->w, "' ", name, "\n", 0);
        }
      }

      buffer_flush(fd_out->w);
    }
    return 0;
  }

  if(argc < 3) {
    builtin_errmsg(argv, "usage", "trap [-lp] [[arg] signal_spec ...]");
    return 2;
  }

  if(str_diff(argv[1], "-")) {
    struct fd fd;
    struct source src;
    struct parser p;

    /* create a new i/o context and initialize a parser */
    source_buffer(&src, &fd, argv[1], str_len(argv[1]));
    parse_init(&p, P_DEFAULT);

    /* parse the string as a compound list */
    if((cmds = parse_compound_list(&p, 0))) {
      /*tree_print(cmds, buffer_2);
      buffer_putnlflush(buffer_2);
      tree_free(cmds);*/
    }

    source_popfd(&fd);
  }

  if((signum = sig_byname(argv[2])) == -1) {
    builtin_errmsg(argv, argv[2], "no such signal");
    if(cmds)
      tree_free(cmds);
    return 1;
  }

  if(signum >= 0) {
    ret = 0;
    if(cmds)
      trap_install(signum, cmds);
    else
      ret = trap_uninstall(signum);
  }

  /*debug_str("builtin_trap", argv[1], 0, '"');
  debug_s("signum: ");
  debug_n(signum);
  debug_nl_fl();*/
  return ret;
}
