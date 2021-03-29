#include "../parse.h"
#include "../source.h"
#include "../tree.h"
#include "../fd.h"
#include "../sh.h"
#include "../../lib/byte.h"

/* parse arithmetic assignment expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_assign(struct parser* p) {
  union node *left, *right, *node;
  enum kind id;
  char c[4] = {0, 0, 0, 0};

  left = parse_arith_value(p);

  if(left->id != N_ARGPARAM)
    return left;

  for(;;) {
    parse_skipspace(p);

    if(source_peek(&c[1]) <= 0)
      return left;

    if(byte_chr("=*/%+-<>&^|", 11, c[1]) < 11) {
      if(c[1] != '=' && source_peekn(&c[2], 1) <= 0)
        return left;
    }

    if(c[1] == '=') {
      id = A_ASSIGN;
    } else if(c[2] == '<' || c[2] == '>') {
    } else if(c[2] == '=') {

      switch(c[1]) {
        case '*': id = A_ASSIGN; break;
        case '/': id = A_ASSIGNDIV; break;
        case '%': id = A_ASSIGNMOD; break;
        case '+': id = A_ASSIGNADD; break;
        case '-': id = A_ASSIGNSUB; break;
        case '&': id = A_ASSIGNBAND; break;
        case '^': id = A_ASSIGNBXOR; break;
        case '|': id = A_ASSIGNBOR; break;
      }
    } else {
      sh_msg("unexpected token ");

      buffer_puts(fd_err->w, c);
      buffer_putnlflush(fd_err->w);
    }

    node = tree_newnode(id);
    node->narithbinary.left = left;
    node->narithbinary.right = parse_arith_assign(p);
    left = node;
  }

  return node;
}
