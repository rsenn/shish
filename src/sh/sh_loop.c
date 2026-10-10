#include "../trace.h"
#include "../eval.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../history.h"
#include "../parse.h"
#include "../prompt.h"
#include "../sh.h"
#include "../source.h"
#include "../tree.h"
#include "../var.h"
#include "../job.h"
#include "builtin_config.h"

#include <unistd.h>

#define IGNOREEOF 10 /* bash's default for $IGNOREEOF */

#include "../trap.h"

/* give back the stdin read-ahead before a command runs, so the command
 * (e.g. `read`) sees the input that follows its own line:
 *
 *   lseek(0, -unread, SEEK_CUR)  then drop the buffer
 *
 * a pipe/tty (ESPIPE) keeps its buffer.
 * ----------------------------------------------------------------------- */
static void
sh_unread_stdin(void) {
  buffer* b = source->b;

  if(source->parent || b->fd != STDIN_FILENO || b->p >= b->n)
    return;

  if(lseek(STDIN_FILENO, -(off_t)(b->n - b->p), SEEK_CUR) != (off_t)-1)
    b->p = b->n = 0;
}

/* main loop, parse lines into trees and execute them
 * ----------------------------------------------------------------------- */
void
sh_loop(void) {
  struct parser p;
  union node* list;
  stralloc cmd;
  int is_interactive = !!(source->mode & SOURCE_IACTIVE);
  int eofs = 0; /* end-of-files in a row that "set -o ignoreeof" has swallowed */

  sh->parser = &p;

  /* if we're in interactive mode some
     additional stuff is to be initialized */
  if(is_interactive) {
    history_init();
  }

  stralloc_init(&cmd);

  parse_init(&p, is_interactive ? P_IACTIVE : P_DEFAULT);

  while(!(parse_gettok(&p, P_DEFAULT) & T_EOF) || (is_interactive && sh->opts.ignoreeof && ++eofs < IGNOREEOF)) {
    /* "set -o ignoreeof": an interactive shell asks for "exit" instead of leaving, but gives up after
       IGNOREEOF end-of-files in a row (a closed stdin would loop forever) */
    if(p.tok & T_EOF) {
      buffer_puts(fd_err->w, "Use \"exit\" to leave the shell.");
      buffer_putnlflush(fd_err->w);
      p.pushback = 0;
      continue;
    }

    eofs = 0;
    p.pushback++;
    sh_errloc_set = 0; /* parse errors report the parser's position */
    parse_lineno = source->position.line;

    var_setvint("LINENO", parse_lineno, V_DEFAULT);

    /* launch the parser to get a complete command */
    list = parse_list(&p);
    stralloc_zero(&cmd);

    {
      char peek;

      if(sh->opts.verbose && !is_interactive && source_peek(&peek) <= 0)
        source_verbose_flush();
    }

    if(list) {
      int status;
      struct eval e;

      TRACE(TRACE_SH, "loop.list", trace_int("n", tree_count(list)), trace_nodes("cmds", list));

      tree_catlist(list, &cmd, NULL);

      /*      if(sh->opts.xtrace) {
              buffer_puts(fd_err->w, "%% ");
              buffer_putsa(fd_err->w, &cmd);
              buffer_putnlflush(fd_err->w);
            }*/

      if(is_interactive)
        history_add(cmd.s, cmd.len);

      /* set -n: fully parse every command but never run any of them.
         POSIX requires interactive shells to ignore this option. */
      if(sh->opts.noexec && !is_interactive) {
        sh->exitcode = 0;
      } else {
        sh_unread_stdin();
        /* directly under "." (E_SOURCE): break/continue may pass on to the caller's loop */
        int under_source = eval && (eval->flags & E_SOURCE);

        eval_push(&e, E_JCTL);
        status = eval_tree(&e, list, E_ROOT | E_LIST | (under_source ? E_EVAL : 0));

        sh->exitcode = eval_pop(&e);
      }

      tree_free(list);
    }

    if(!(p.tok & (T_NL | T_SEMI | T_BGND))) {
      /* we have a parse error */
      if(p.tok != T_EOF)
        parse_error(&p, 0);

      /* exit if not interactive -- a clean T_EOF right after the last
         command (no trailing newline/semicolon, as with a "-c"
         argument) is not itself an error, so it must exit with that
         command's own status rather than a hardcoded 0.
         sh_interactive (the whole session's), not source->mode's
         per-buffer SOURCE_IACTIVE -- otherwise a `.`-sourced file
         missing its own trailing newline would end the entire
         session instead of just returning to its caller. */
      if(!sh_interactive)
        sh_exit(p.tok != T_EOF ? 1 : sh->exitcode);

      /* ..otherwise discard the input buffer */
      source_flush();
      p.pushback = 0;
    }

    if(p.tok & (T_NL | T_SEMI | T_BGND))
      p.pushback = 0;

    job_update();

    /* the common, low-latency dispatch point for a real-signal trap
       whose signal fired while shish itself was busy (not blocked in
       job_wait(), which has its own matching call) doing ordinary,
       uninterruptible work between statements. */
    trap_run_pending();

    /* reset prompt */
    prompt_reset();
  }
}
