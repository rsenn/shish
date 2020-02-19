#include "../parse.h"
#include "../tree.h"
#include "../source.h"

/* parse a compound- or a simple-command
 * (pipeline and lists are done outside this)
 * ----------------------------------------------------------------------- */
union node*
parse_command(struct parser* p, int tempflags) {
  enum tok_flag tok;
  union node* command;
  union node** rptr;
  char c = 0;

  tok = parse_gettok(p, tempflags);

  //  source_skipspace(p);
  while(source_peek(&c) >= 1) {
    if(!parse_isspace(c))
      break;
    source_skip();
  }

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
      if(c == '(') {
        char ch[2];
        if(source_peekn(ch, 1) >= 2 && ch[1] == ')') {
          source_skip();
          source_skip();
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
    while(*rptr) tree_next(rptr);

    while(parse_gettok(p, P_DEFAULT) & T_REDIR) tree_move(p->tree, rptr);
  }

  p->pushback++;

  return command;
}
