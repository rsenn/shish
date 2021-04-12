#include "../builtin.h"
#include "../debug.h"
#include "../tree.h"
#include "../parse.h"
#include "../source.h"
#include "../../lib/sig.h"

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_trap(int argc, char* argv[]) {
  union node* cmds = 0;
  int signum = -1;

  if(argc > 1) {
    struct fd fd;
    struct source src;
    struct parser p;

    /* create a new i/o context and initialize a parser */
    source_buffer(&src, &fd, argv[1], str_len(argv[1]));
    parse_init(&p, P_DEFAULT);

    /* parse the string as a compound list */
    if((cmds = parse_compound_list(&p, 0))) {
      tree_print(cmds, buffer_2);
      buffer_putnlflush(buffer_2);
      tree_free(cmds);
    }

    source_popfd(&fd);
  }

  if(argc > 2) {
    signum = sig_byname(argv[2]);
  }

  debug_str("builtin_trap", argv[1], 0, '"');
  debug_s("signum: ");
  debug_n(signum);
  debug_nl_fl();
  return 0;
}
