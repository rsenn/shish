#include "parse.h"
#include "source.h"
#include "tree.h"

/* parse arithmetic binary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_binary(struct parser* p, int precedence) {
  union node *lnode, *rnode, *newnode;
  char c;
  int ntype = -1;
  char cbuf[2];
  int prec = precedence;

  lnode = precedence < 1 ? parse_arith_unary(p) : parse_arith_binary(p, precedence - 1);

  if(lnode == NULL)
    return NULL;

  parse_skipspace(p);

  if(source_peek(&cbuf[0]) <= 0 || source_peekn(&cbuf[1], 1) <= 0) {
    tree_free(lnode);
    return NULL;
  }

  c = cbuf[0];

  do {

    if(prec <= 0)
      break;

    if(prec <= 1) {
      switch(c) {
        case '*': ntype = N_ARITH_MUL; break;
        case '/': ntype = N_ARITH_DIV; break;
      }
    } else if(prec <= 2) {
      switch(c) {
        case '+': ntype = N_ARITH_ADD; break;
        case '-': ntype = N_ARITH_SUB; break;
      }
    } else if(prec <= 3) {

      if((cbuf[0] == '>' || cbuf[0] == '<') && cbuf[0] == cbuf[1]) {
        ntype = cbuf[0] == '>' ? N_ARITH_RSHIFT : N_ARITH_LSHIFT;
        source_skip();
      }

    } else if(prec <= 4) {

      if(cbuf[0] == '>') {
        ntype = cbuf[1] == '=' ? N_ARITH_GE : N_ARITH_GT;
        if(cbuf[1] == '=')
          source_skip();
      } else if(cbuf[0] == '<') {
        ntype = cbuf[1] == '=' ? N_ARITH_LE : N_ARITH_LT;
        if(cbuf[1] == '=')
          source_skip();
      } else if(cbuf[1] == '=' && (cbuf[0] == '=' || cbuf[0] == '!')) {
        ntype = cbuf[0] == '!' ? N_ARITH_NE : N_ARITH_EQ;
        source_skip();
      }
    } else if(prec <= 5) {
      switch(c) {
        case '&': ntype = N_ARITH_BAND; break;
        case '|': ntype = N_ARITH_BOR; break;
        case '^': ntype = N_ARITH_BXOR; break;
      }
    } else if(prec <= 6) {
      if((cbuf[0] == '&' || cbuf[0] == '|') && cbuf[0] == cbuf[1]) {
        ntype = cbuf[0] == '&' ? N_ARITH_AND : N_ARITH_OR;
        source_skip();
      }
    }
    --prec;
  } while(ntype == -1);

  if(ntype == -1)
    return lnode;

  source_skip();
  parse_skipspace(p);

  rnode = precedence < 1 ? parse_arith_unary(p) : parse_arith_binary(p, precedence - 1);

  if(rnode == NULL) {
    tree_free(lnode);
    return NULL;
  }

  newnode = tree_newnode(ntype);
  newnode->narithbinary.left = lnode;
  newnode->narithbinary.right = rnode;

  return newnode;
}
