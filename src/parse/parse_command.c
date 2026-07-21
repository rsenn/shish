#include "../parse.h"
#include "../tree.h"
#include "../source.h"
#include "../fd.h"
#include "../sh.h"
#include "../debug.h"

/* parse a compound- or a simple-command
 * (pipeline and lists are done outside this)
 * ----------------------------------------------------------------------- */
union node*
parse_command(struct parser* p, int tempflags) {
  enum tok_flag tok;
  union node *command = 0, **rptr;
  char c = 0;

  tok = parse_gettok(p, tempflags);

  // source_skipspace(p);
  /*  while(source_peek(&c) >= 1) {
      if(!parse_isspace(c))
        break;
      parse_skip(p);
    }

*/
  switch(tok) {
    /* T_FOR begins an iteration statement */
    case T_FOR: command = parse_for(p); break;

    /* T_IF begins a case match statement */
    case T_CASE: command = parse_case(p); break;

    /* T_IF begins a conditional statement */
    case T_IF: command = parse_if(p); break;

    /* T_WHILE/T_UNTIL begin a for-loop statement */
    case T_WHILE:
    case T_UNTIL: command = parse_loop(p); break;

    /* T_LP/T_BEGIN start a grouping compound. Backgrounding a compound
       command is handled uniformly for every node kind by parse_list()
       (it just sets ->ngrp.bgnd on whatever and-or-list it got back),
       so this used to duplicate that with a broken shortcut: peeking
       p->tok directly (bypassing the normal push-back-and-re-read
       protocol) and then calling parse_gettok(p, 0) to "consume" it.
       Since parse_grouping() leaves the '&' pending via pushback (not
       yet re-read), that call didn't advance to a new token -- it just
       returned the still-cached '&' and cleared pushback. With
       pushback now clear, the redirection-lookahead loop right below
       genuinely read the *next* token (the start of the following
       command) as its lookahead and left it pushed back instead,
       which parse_list() then saw in place of the '&' it needed,
       terminating the list early and leaking that command's first
       token as bogus leftover state. Repro: "{ true; } & echo after"
       ("unexpected token NAME" right after the "}"). */
    case T_LP:
    case T_BEGIN:
      p->pushback++;
      command = parse_grouping(p, 0);
      break;

    /* handle simple commands */
    case T_NAME:
      parse_skipspace(p);

      if(source_peek(&c) > 0 && c == '(') {
        char ch[2];

        if(source_peekn(ch, 1) <= 0)
          return NULL;

        if(ch[0] == ')') {
          parse_skip(p);
          parse_skip(p);
          p->pushback++;
          return parse_function(p);
        }
      }

    case T_WORD:
    case T_REDIR:
    case T_ASSIGN:
      p->pushback++;
      command = parse_simple_command(p);
      break;

    /* it wasn't a compound command, return now */
    default:
      command = 0;
      p->pushback++;
      return NULL;
  }

  if(command) {
    /* they all can have redirections, so parse these now */
    rptr = &command->ncmd.rdir;

    /*
     * in the case of a simple command there are maybe already
     * redirections in the list (because in a simple command they
     * can appear between arguments), so skip to the end of the list.
     */
    while(*rptr)
      tree_skip(rptr);

    while(parse_gettok(p, P_DEFAULT) & T_REDIR)
      tree_move(p->tree, rptr);
  }

  p->pushback++;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE)
  buffer_putm_internal(debug_output, COLOR_YELLOW "parse_command" COLOR_NONE " command = ", 0);
  debug_node(command, 1);
  debug_nl_fl();
#endif

  return command;
}
