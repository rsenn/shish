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
#include "../job.h"

/* main loop, parse lines into trees and execute them
 * ----------------------------------------------------------------------- */
void
sh_loop(void) {
  struct parser p;
  union node* list;
  stralloc cmd;
  int is_interactive = !!(source->mode & SOURCE_IACTIVE);

  sh->parser = &p;

  /* if we're in interactive mode some
     additional stuff is to be initialized */
  if(is_interactive) {
    history_load();
  }

  stralloc_init(&cmd);

  parse_init(&p, is_interactive ? P_IACTIVE : P_DEFAULT);

  while(!(parse_gettok(&p, P_DEFAULT) & T_EOF)) {
    p.pushback++;
    parse_lineno = source->position.line;

    var_setvint("LINENO", parse_lineno, V_DEFAULT);

    /* launch the parser to get a complete command */
    list = parse_list(&p);
    stralloc_zero(&cmd);

    if(list) {
      int status;
      struct eval e;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE) && !defined(SHPARSE2AST)
      /* tree_print(list, buffer_2);
       buffer_putnlflush(buffer_2);*/

      buffer_puts(debug_output, COLOR_YELLOW "sh_loop" COLOR_NONE " list = ");
      debug_list(list, 0);
      debug_nl_fl();
#endif

      tree_catlist(list, &cmd, NULL);

      /*      if(sh->opts.xtrace) {
              buffer_puts(fd_err->w, "%% ");
              buffer_putsa(fd_err->w, &cmd);
              buffer_putnlflush(fd_err->w);
            }*/

      if(is_interactive) {
        stralloc_nul(&cmd);
        history_set(cmd.s);
        cmd.s = NULL;
        history_advance();
      }

      eval_push(&e, E_JCTL);
      status = eval_tree(&e, list, E_ROOT | E_LIST);

      // eval_pop(&e);
      // while(sh->eval != &e) eval_pop(sh->eval);

      sh->exitcode = eval_pop(&e);

      tree_free(list);
    }

    if(!(p.tok & (T_NL | T_SEMI | T_BGND))) {
      /* we have a parse error */
      if(p.tok != T_EOF)
        parse_error(&p, 0);

      /* exit if not interactive */
      if(!(source->mode & SOURCE_IACTIVE))
        sh_exit(p.tok != T_EOF);

      /* ..otherwise discard the input buffer */
      source_flush();
      p.pushback = 0;
    }

    if(p.tok & (T_NL | T_SEMI | T_BGND))
      p.pushback = 0;

    job_clean();

    /* reset prompt */
    prompt_reset();
  }
}
