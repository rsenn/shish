#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parse arithmetic binary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_binary(struct parser* p, int precedence) {
  union node *left, *right, *node;
  int op = -1;
  char a, b, c;

  left = precedence <= 1 ? parse_arith_unary(p) : parse_arith_binary(p, precedence - 1);

  if(left == NULL)
    return NULL;

  parse_skipspace(p);

  if(source_peek(&a) <= 0 || source_peekn(&b, 1) <= 0) {
    // tree_free(left);
    return left;
  }

  do {
    if(precedence <= 0)
      break;

    if(precedence <= 9) {
      if((a == '-' || a == '+') && b == a)
        return NULL;
    }

    if(precedence <= 1) {
      if(a == '*' && b == '*') {
        parse_skip(p);
        op = A_EXP;
      }
    } else if(precedence <= 2) {
      if(b != '=')
        switch(a) {
          case '*': op = A_MUL; break;
          case '/': op = A_DIV; break;
          case '%': op = A_MOD; break;
        }
    } else if(precedence <= 3) {
      if(b != '=')
        switch(a) {
          case '+': op = A_ADD; break;
          case '-': op = A_SUB; break;
        }
    } else if(precedence <= 4) {
      if(source_peekn(&c, 2) <= 0)
        return left;

      if(c != '=')
        if((a == '>' || a == '<') && a == b) {
          op = a == '>' ? A_SHR : A_SHL;
          parse_skip(p);
        }

    } else if(precedence <= 5) {

      if(a == '>') {
        op = b == '=' ? A_GE : A_GT;

        if(b == '=')
          parse_skip(p);
      } else if(a == '<') {
        op = b == '=' ? A_LE : A_LT;

        if(b == '=')
          parse_skip(p);
      }

    } else if(precedence <= 6) {

      if(b == '=' && (a == '=' || a == '!')) {
        op = a == '!' ? A_NE : A_EQ;
        parse_skip(p);
      }
    } else if(precedence <= 7) {
      if(b != '=')
        switch(a) {
          case '&': op = A_BITAND; break;
          case '|': op = A_BITOR; break;
          case '^': op = A_BITXOR; break;
        }
    } else if(precedence <= 8) {
      if((a == '&' || a == '|') && a == b) {
        op = a == '&' ? A_AND : A_OR;
        parse_skip(p);
      }
    }

    --precedence;
  } while(op == -1);

  if(op == -1)
    return left;

  parse_skip(p);
  parse_skipspace(p);

  right = precedence <= 1 ? parse_arith_unary(p) : parse_arith_binary(p, precedence - 1);

  if(right == NULL)
    right = parse_arith_value(p);

  if(right == NULL)
    return left;
  /*
    tree_free(left);
    //parse_gettok(p, P_DEFAULT);

    //sh_msg("no")
    //parse_error(p, 0);
    return 0;
  */

  node = tree_newnode(op);
  node->narithbinary.left = left;
  node->narithbinary.right = right;

  return node;
}
