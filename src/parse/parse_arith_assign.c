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

  if((left = parse_arith_value(p)) == 0)
    return 0;

  if(left->id != N_ARGPARAM)
    return left;

  for(;;) {
    parse_skipspace(p);

    if(source_peek(&c[0]) <= 0)
      break;

    if(byte_chr("=*/%+-<>&^|", 11, c[0]) == 11)
      break;

    if(c[0] != '=' && source_peekn(&c[1], 1) <= 0)
      break;

    if(c[0] == '=') {
      id = A_ASSIGN;
      source_skip();
      break;
    } else if((c[0] == '<' || c[0] == '>') && c[0] == c[1]) {

      if(source_peekn(&c[2], 2) <= 0 || c[2] != '=')
        break;

      id = c[0] == '<' ? A_ASSIGNLSHIFT : A_ASSIGNRSHIFT;
        source_skip();
        source_skip();
        source_skip();

    } else if(c[1] == '=') {

      switch(c[0]) {
        case '*': id = A_ASSIGNMUL; break;
        case '/': id = A_ASSIGNDIV; break;
        case '%': id = A_ASSIGNMOD; break;
        case '+': id = A_ASSIGNADD; break;
        case '-': id = A_ASSIGNSUB; break;
        case '&': id = A_ASSIGNBAND; break;
        case '^': id = A_ASSIGNBXOR; break;
        case '|': id = A_ASSIGNBOR; break;
        default: c[0] = '\0'; break;
      }
      if(c[0]) {
        source_skip();
        source_skip();
      }
    } else {
      c[0] = '\0';
      /*      sh_msg("unexpected token ");

            buffer_puts(fd_err->w, c);
            buffer_putnlflush(fd_err->w);
            tree_free(left);
            return 0;
            break;*/
    }
    if(!c[0])
      break;

    parse_skipspace(p);
    right = parse_arith_binary(p, 9);

    node = tree_newnode(id);
    node->narithbinary.left = left;
    node->narithbinary.right = right;
    left = node;
  }

  return left;
}
