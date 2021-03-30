#include "../expand.h"
#include "../fd.h"
#include "../parse.h"
#include "../source.h"
#include "../tree.h"
#include "../../lib/uint16.h"
#include "../../lib/uint64.h"
#include "../../lib/fmt.h"
#include "../../lib/byte.h"
#include "../../lib/scan.h"
#include <assert.h>

typedef size_t scan_function(const char*, int64*);

/* parse arithmetic value
 * ----------------------------------------------------------------------- */
union node*
parse_arith_value(struct parser* p) {
  char c;
  uint16 digit, classes;
  int cclass = C_DIGIT;

  union node* node = NULL;

  if(source_peek(&c) <= 0)
    return NULL;

  if(c == '(')
    return parse_arith_paren(p);

  if(parse_isdigit(c)) {
    char x[FMT_LONG + 1];
    unsigned base = 10;
    size_t n = 0;
    int64 num;

    do {
      x[n++] = c;

      if(source_next(&c) <= 0)
        break;

      classes = parse_chartable[(int)(unsigned char)c];
      digit = !!(classes & cclass);

      if(n == 1 && (cclass == C_DIGIT)) {

        if(x[0] == '0') {
           /*f((classes & C_OCTAL))
            c = 'o';*/

          switch(c) {
            case 'x':
              cclass = C_HEX;
              base = 16;
              break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
              cclass = C_OCTAL;
              base = 8;
              break;
            default:
              buffer_puts(fd_err->w, "ERROR: expecting x|b|o");
              buffer_putnlflush(fd_err->w);
              return NULL;
          }

          if((classes & (C_UPPER | C_LOWER))) {
            digit = source_next(&c) > 0;
            n = 0;
          }
        }
      }
    } while(digit && n < FMT_LONG);

    x[n] = '\0';

    switch(base) {
      case 8: scan_8longlong(x, &num); break;
      case 16: scan_xlonglong(x, &num); break;
      case 10:
      default: scan_longlong(x, &num); break;
    }

    node = tree_newnode(A_NUM);
    node->narithnum.num = num;
    node->narithnum.base = base;

    return node;
  }

  if(parse_isalpha(c) || c == '_' || c == '$') {
    source_peekn(&c, 1);
    if((byte_chr("({", 2, c) < 2 || parse_isname(c, 0))) {
      parse_subst(p);
      node = p->tree;
      p->node = p->tree = NULL;
      if(node->id == N_ARGPARAM)
        node->nargparam.flag |= S_ARITH;
      return node;
    }
  }

  return NULL;
}