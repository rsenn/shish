#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parse arithmetic unary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_unary(struct parser* p) {
  union node *node = 0, *unary;
  enum kind n;
  char c, c2 = 0;

  if(source_peek(&c) <= 0)
    return 0;

  if((c == '+' || c == '-')) {
    if(source_peekn(&c2, 1) <= 0)
      return 0;
  }

  switch(c) {
    case '!': n = A_NOT; break;
    case '~': n = A_BNOT; break;
    case '-': n = c2 == '-' ? A_PREDECR : A_UNARYMINUS; break;
    case '+': n = c2 == '+' ? A_PREINCR : A_UNARYPLUS; break;
    default: {
      if((node = parse_arith_value(p))) {
        if(source_peek(&c) > 0 && (c == '+' || c == '-')) {
          if(source_peekn(&c2, 1) > 0 && c2 == c) {
            switch(c) {
              case '-': n = A_POSTDECR; break;
              case '+': n = A_POSTINCR; break;
            }

            break;
          }
        }
      }
      return node;
    }
  }

  if(c == c2)
    parse_skip(p);

  parse_skip(p);
  parse_skipspace(p);

  unary = tree_newnode(n);
  unary->narithunary.node = node ? node : parse_arith_value(p);

  return unary;
}
