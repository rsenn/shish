#include "../debug.h"
#include "../eval.h"
#include "../fd.h"
#include "../history.h"
#include "../parse.h"
#include "../prompt.h"
#include "../sh.h"
#include "../source.h"
#include "../tree.h"
#include "../var.h"

/* main loop, parse lines into trees and execute them
 * ----------------------------------------------------------------------- */
void
sh_loop(void) {
  struct parser p;
  union node* list;
  stralloc cmd;

  /* if we're in interactive mode some
     additional stuff is to be initialized */
  if(source->mode & SOURCE_IACTIVE)
    history_load();

  stralloc_init(&cmd);

  parse_init(&p, P_DEFAULT);

  while(!(parse_gettok(&p, P_DEFAULT) & T_EOF)) {
    p.pushback++;
    parse_lineno = source->line;

    var_setvint("LINENO", parse_lineno, V_DEFAULT);

    /* launch the parser to get a complete command */
    if((list = parse_list(&p))) {
      struct eval e;

#if(defined(_DEBUG) && !defined(NO_TREE_PRINT)) || defined(SHFMT)
      stralloc_zero(&cmd);
      tree_catlist(list, &cmd, NULL);

      buffer_putsa(fd_out->w, &cmd);
      buffer_putnlflush(fd_out->w);
#endif

      if(source->mode & SOURCE_IACTIVE) {
        buffer* in = source->b;

        stralloc_copyb(&cmd, in->x, in->n);

        if(!(cmd.len > 0 && cmd.s[cmd.len - 1] == '\n'))
          stralloc_catc(&cmd, '\n');

        stralloc_nul(&cmd);
        history_set(cmd.s);
        cmd.s = NULL;
        history_advance();
      }

#if defined(DEBUG_OUTPUT)
      if(sh->flags & SH_DEBUG) {
        debug_list(list, 0);
        buffer_putnlflush(fd_err->w);
      }
#endif /* DEBUG_OUTPUT */

#ifndef SHFMT
      eval_push(&e, E_JCTL);
      eval_tree(&e, list, E_ROOT | E_LIST);
      sh->exitcode = eval_pop(&e);
#endif
      stralloc_zero(&cmd);

      tree_free(list);
    } else if(!(p.tok & (T_NL | T_SEMI | T_BGND))) {
      /* we have a parse error */
      if(p.tok != T_EOF)
        parse_error(&p, 0);

      /* exit if not interactive */
      if(!(source->mode & SOURCE_IACTIVE))
        sh_exit(1);

      /* ..otherwise discard the input buffer */
      source_flush();
      p.pushback = 0;
    }

    if(p.tok & (T_NL | T_SEMI | T_BGND))
      p.pushback = 0;

    /* reset prompt */
    prompt_reset();
  }
}
