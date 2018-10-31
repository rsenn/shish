#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic parentheses expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_paren(struct parser *p) {
  char c;
  union node *node, **nptr;

  if(source_peek(&c) <= 0)
    return NULL;

  if(c != '(') return NULL;

  source_skip();
  parse_skipspace(p);

  node = tree_newnode(N_ARITH_PAREN);

  nptr = &node->nargarith.tree;

again: 
    if((*nptr =  parse_arith_expr(p)) == NULL)
      return NULL;

    nptr = &(*nptr)->nargarith.next;

    if(source_peek(&c) <= 0) return NULL;

    if(c == ',') {
      source_skip();
      goto again;
    }
    
     if(c != ')') {
       asm("int3");
    }

  source_skip();

  return node;
}
