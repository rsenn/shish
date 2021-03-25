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
  union node* command = 0;
  union node** rptr;
  char c = 0;

  tok = parse_gettok(p, tempflags);

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE) && !defined(SHFORMAT) && !defined(SHPARSE2AST)
  if(sh->opts.debug) {
    debug_begin("parse_command", 0);
    debug_str("tok", parse_tokname(tok, 1), 0, 0);
    debug_nl_fl();
  }
#endif

  //  source_skipspace(p);
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

    /* T_LP/T_BEGIN start a grouping compound */
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
    while(*rptr) tree_skip(rptr);

    while(parse_gettok(p, P_DEFAULT) & T_REDIR) tree_move(p->tree, rptr);
  }

  p->pushback++;

  return command;
}
