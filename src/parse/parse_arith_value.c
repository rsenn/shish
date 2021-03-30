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

typedef size_t scan_function(const char*, int64*);

/* parse arithmetic value
 * ----------------------------------------------------------------------- */
union node*
parse_arith_value(struct parser* p) {
  char buf[3];
  uint16 digit, classes;
  int cclass = C_DIGIT;
  unsigned base = 10;
  scan_function* scan_fn;

  union node* node = NULL;

  if(source_peek(&buf[0]) <= 0)
    return NULL;

  if(buf[0] == '(')
    return parse_arith_paren(p);

  if(parse_isdigit(buf[0])) {
    char x[FMT_LONG + 1];

    size_t n = 0;
    int64 num;

    do {
      x[n++] = buf[0];

      if(source_next(&buf[1]) <= 0)
        break;

      classes = parse_chartable[(int)(unsigned char)buf[1]];
      digit = !!(classes & cclass);

      if(n == 1 && (cclass == C_DIGIT && x[0] == '0')) {

        if((classes & C_OCTAL))
          buf[1] = 'o';

        switch(buf[1]) {
          case 'x':
            cclass = C_HEX;
            base = 16;
            break;
          case 'o':
            cclass = C_OCTAL;
            base = 8;
            break;
          default:
            buffer_puts(fd_err->w, "ERROR: expecting x|b|o");
            buffer_putnlflush(fd_err->w);
            return NULL;
        }

        if((classes & (C_UPPER | C_LOWER))) {
          digit = source_next(&buf[2]) > 0;
          n = 0;
        }
      }

    } while(digit && n < FMT_LONG);

    x[n] = '\0';

    switch(base) {
      case 8: scan_fn = (scan_function*)&scan_8longlong; break;
      case 10: scan_fn = (scan_function*)&scan_longlong; break;
      case 16: scan_fn = (scan_function*)&scan_xlonglong; break;
    }

    scan_fn(x, &num);

    node = tree_newnode(A_NUM);
    node->narithnum.num = num;
    node->narithnum.base = base;

    return node;
  }

  if(parse_isalpha(buf[0]) || buf[0] == '_' || buf[0] == '$') {
    source_peekn(&buf[1], 1);
    if((byte_chr("({", 2, buf[1]) < 2 || parse_isname(buf[1], 0)) && !parse_subst(p)) {
      node = p->tree;
      p->node = p->tree = NULL;

      if(node->id == N_ARGPARAM)
        node->nargparam.flag |= S_ARITH;

      return node;
    }
  }

  return NULL;
}