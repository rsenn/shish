#include "../parse.h"
#include "../tree.h"
#include "../expand.h"
#include "../fd.h"
#include "../debug.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"

/* 3.9.5 - parse function definition
 *
 * POSIX allows any compound command as the function body:
 * f() { ...; }      - brace group
 * f() ( ... )       - subshell
 * f() for ... done  - for loop
 * f() while ... done - while loop
 * f() until ... done - until loop
 * f() if ... fi     - if statement
 * f() case ... esac - case statement
 * ----------------------------------------------------------------------- */
union node*
parse_function(struct parser* p) {
  enum tok_flag tok;
  union node* node;
  struct location loc;
  struct nargstr* argstr;

  stralloc name;
  stralloc_init(&name);
  tok = parse_gettok(p, P_DEFAULT);
  argstr = &p->tree->nargstr;
  loc = argstr->loc;
  stralloc_copy(&name, &argstr->stra);

  // expand_tosa(p->tree, &name);
  node = tree_newnode(N_FUNCTION);
  stralloc_nul(&name);
  node->nfunc.name = name.s;

  do {
    tok = parse_gettok(p, P_DEFAULT);
  } while(tok == T_NL || tok == T_NAME);

  p->pushback++;
  
  /* POSIX: function body can be any compound command, not just {...} or (...) */
  tok = parse_gettok(p, P_SKIPNL);

  if(tok == T_BEGIN || tok == T_LP) {
    /* Traditional brace group or subshell - push back and call parse_grouping */
    p->pushback++;
    node->nfunc.body = parse_grouping(p, P_SKIPNL);
  } else {
    /* Any other compound command (for, while, until, if, case) */
    /* Push back the token and call parse_command to handle it */
    p->pushback++;
    node->nfunc.body = parse_command(p, 0);

    if(!node->nfunc.body) {
      /* parse_command failed */
      tree_free(node);
      return NULL;
    }
  }
  
  node->nfunc.loc = loc;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE) && !defined(SHPARSE2AST)
  buffer_puts(debug_output, COLOR_YELLOW "parse_function" COLOR_NONE " node = ");
  debug_node(node, 1);
  debug_nl_fl();
#endif

  return node;
}
