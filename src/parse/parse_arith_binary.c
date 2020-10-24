#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parse arithmetic binary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_binary(struct parser* p, int precedence) {
  union node *lnode, *rnode, *newnode;
  char c;
  int ntype = -1;
  char a, b;
  int prec = precedence;

  lnode = precedence < 1 ? parse_arith_unary(p) : parse_arith_binary(p, precedence - 1);

  if(lnode == NULL)
    return NULL;

  parse_skipspace(p);

  if(source_peek(&a) <= 0 || source_peekn(&b, 1) <= 0) {
    // tree_free(lnode);
    return lnode;
  }

  c = a;

  do {
    if(prec <= 0)
      break;

    if(prec <= 9) {
      if((a == '-' || a == '+') && b == a)
        return NULL;
    }

    if(prec <= 1) {
      if(a == '*' && b == '*') {
        parse_skip(p);
        ntype = A_EXP;
      }
    } else if(prec <= 2) {
      switch(c) {
        case '*': ntype = A_MUL; break;
        case '/': ntype = A_DIV; break;
        case '%': ntype = A_MOD; break;
      }
    } else if(prec <= 3) {
      switch(c) {
        case '+': ntype = A_ADD; break;
        case '-': ntype = A_SUB; break;
      }
    } else if(prec <= 4) {

      if((a == '>' || a == '<') && a == b) {
        ntype = a == '>' ? A_RSHIFT : A_LSHIFT;
        parse_skip(p);
      }

    } else if(prec <= 5) {

      if(a == '>') {
        ntype = b == '=' ? A_GE : A_GT;
        if(b == '=')
          parse_skip(p);
      } else if(a == '<') {
        ntype = b == '=' ? A_LE : A_LT;
        if(b == '=')
          parse_skip(p);
      }

    } else if(prec <= 6) {

      if(b == '=' && (a == '=' || a == '!')) {
        ntype = a == '!' ? A_NE : A_EQ;
        parse_skip(p);
      }
    } else if(prec <= 7) {
      switch(c) {
        case '&': ntype = A_BAND; break;
        case '|': ntype = A_BOR; break;
        case '^': ntype = A_BXOR; break;
      }
    } else if(prec <= 8) {
      if((a == '&' || a == '|') && a == b) {
        ntype = a == '&' ? A_AND : A_OR;
        parse_skip(p);
      }
    }
    --prec;
  } while(ntype == -1);

  if(ntype == -1)
    return lnode;

  parse_skip(p);
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
