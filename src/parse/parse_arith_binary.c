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

      /* "<<"/">>" belong to the tighter shift level below and must
         not be mistaken for a chained "<"/">" here -- same class of
         bug as the "&"/"&&" one above: a second, still-unconsumed
         shift operator (e.g. the trailing "<<1" of "1<<2<<1", once
         the first "1<<2" has already been parsed as the shift level's
         own node and returned up as this frame's "left") otherwise
         got read as a bare "<"/">" comparison instead of being left
         for the shift level to have handled via its own chaining. */
      if(a == '>' && b != '>') {
        op = b == '=' ? A_GE : A_GT;

        if(b == '=')
          parse_skip(p);
      } else if(a == '<' && b != '<') {
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
      /* "&&"/"||" belong to the next (looser) precedence level below --
         without excluding b == a here, a bitwise "&"/"|" check run
         against the *first* character of "&&"/"||" matched too (b was
         only checked against '=', never against a repeat of a itself),
         so "3&&-5" got mis-split into a one-character bitwise-AND
         ("3&") plus a dangling "&-5" instead of ever reaching the
         logical-AND branch below. */
      if(b != '=' && !((a == '&' || a == '|') && b == a))
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

    /* stop *before* decrementing once a match is found -- otherwise
       "precedence" (used just below to bound the right operand's own
       recursion) ends up one level looser than the level that was
       actually matched, e.g. "+" matching immediately at its own
       precedence=3 still fell through to "--precedence" first, so the
       right operand recursed at precedence 1 (exponent only) instead
       of 2 (multiplicative) and never picked up a trailing "*3" --
       left for an ancestor frame to instead re-group as
       "(1+2)*3" instead of "1+(2*3)". */
    if(op != -1)
      break;

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
