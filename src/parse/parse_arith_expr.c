#include "parse.h"
#include "source.h"
#include "tree.h"

/* parse arithmetic expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_expr(struct parser* p) {
  union node* node;
  char c = 0, c2 = 0;

  parse_skipspace(p);

  node = parse_arith_binary(p, 9);
  if(!node)
    node = parse_arith_unary(p);
  if(!node) {
    node = parse_arith_value(p);

    if(node) {
      if(node->id == N_ARGPARAM) {
        if(source_peek(&c) <= 0 || source_peekn(&c2, 1) <= 0) {
          return 0;

          if((c == '+' || c == '-') && c == c2) {
            union node* post = tree_newnode(c == '+' ? A_POSTINCREMENT : A_POSTDECREMENT);
            source_skip();
            source_skip();

            post->narithunary.node = node;
            node = post;
          }
        }
      }
    }
  }

  return node;
}
