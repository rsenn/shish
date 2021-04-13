#include "../parse.h"
#include "../tree.h"
#include "../expand.h"
#include "../fd.h"
#include "../debug.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"

/* 3.9.5 - parse function definition
 * ----------------------------------------------------------------------- */
union node*
parse_function(struct parser* p) {
  int tok;
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

  do
    tok = parse_gettok(p, P_DEFAULT);
  while(tok == T_NL || tok == T_NAME);
  p->pushback++;
  /*
    if((tok = parse_gettok(p, 0)) != T_BEGIN) {
      tree_free(node);
      return NULL;
    }*/
  if(!parse_expect(p, P_SKIPNL, T_BEGIN | T_LP, node))
    return NULL;

  p->pushback++;
  node->nfunc.body = parse_grouping(p, P_SKIPNL);
  node->nfunc.loc = loc;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE) && !defined(SHPARSE2AST)
  buffer_puts(debug_output, COLOR_YELLOW "parse_function" COLOR_NONE " node = ");
  debug_node(node, 1);
  debug_nl_fl();
#endif

  return node;
}
