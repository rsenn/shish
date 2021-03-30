#include "../parse.h"
#include "../source.h"
#include "../tree.h"
#include "../fd.h"
#include "../sh.h"
#include "../../lib/byte.h"

/* parse arithmetic assignment expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_assign(struct parser* p, union node* left) {
  union node *right, *node = 0, **nptr;
  enum kind id;
  char c[4] = {0, 0, 0, 0};

  /*  if(left == 0)
      if((left = parse_arith_binary(p, 9)) == 0 || left->id != N_ARGPARAM)
        return left;*/

  for(nptr = &node;; left = 0) {
    if(left == 0) {
      if((left = parse_arith_binary(p, 9)) == 0)
        break;
      if(left->id != N_ARGPARAM) {
        *nptr = left;
        return node;
      }
    }

    parse_skipspace(p);

    if(source_peek(&c[0]) <= 0)
      break;

    if(byte_chr("=*/%+-<>&^|", 11, c[0]) == 11)
      break;

    if(c[0] != '=' && source_peekn(&c[1], 1) <= 0)
      break;

    if(c[0] == '=') {
      id = A_VASSIGN;
      source_skip();
    } else if((c[0] == '<' || c[0] == '>') && c[0] == c[1]) {

      if(source_peekn(&c[2], 2) <= 0 || c[2] != '=')
        break;

      id = c[0] == '<' ? A_VSHL : A_VSHR;
      source_skip();
      source_skip();
      source_skip();

    } else if(c[1] == '=') {

      switch(c[0]) {
        case '*': id = A_VMUL; break;
        case '/': id = A_VDIV; break;
        case '%': id = A_VMOD; break;
        case '+': id = A_VADD; break;
        case '-': id = A_VSUB; break;
        case '&': id = A_VBITAND; break;
        case '^': id = A_VBITXOR; break;
        case '|': id = A_VBITOR; break;
        default: c[0] = '\0'; break;
      }
      if(c[0]) {
        source_skip();
        source_skip();
      }
    } else {
      break;
    }

    parse_skipspace(p);

    *nptr = tree_newnode(id);
    (*nptr)->narithbinary.left = left;
    // l
    left = *nptr;
    nptr = &(*nptr)->narithbinary.right;
  }

  if(nptr && *nptr == 0)
    *nptr = parse_arith_binary(p, 9);

  return node;
}
