#include "expand.h"
#include "fd.h"
#include "fmt.h"
#include "parse.h"
#include "scan.h"
#include "source.h"
#include "tree.h"
#include "uint16.h"
#include "uint64.h"

typedef size_t scan_function(const char*, int64*);

/* parse arithmetic value
 * ----------------------------------------------------------------------- */
union node*
parse_arith_value(struct parser* p) {
  char c;
  uint16 digit, classes;
  int cclass = C_DIGIT;

  scan_function* scan_fn = &scan_longlong;

  union node* node = NULL;

  if(source_peek(&c) <= 0)
    return NULL;

  if(c == '(')
    return parse_arith_paren(p);

  if(parse_isdigit(c)) {
    char x[FMT_LONG + 1];

    size_t n = 0;
    int64 num;

    do {
      x[n++] = c;

      if(source_next(&c) <= 0)
        break;

      classes = parse_chartable[(int)(unsigned char)c];
      digit = !!(classes & cclass);

      if(n == 1 && (cclass == C_DIGIT && x[0] == '0')) {

        if((classes & C_OCTAL))
          c = 'o';

        switch(c) {
          case 'x':
            scan_fn = (scan_function*)&scan_xlonglong;
            cclass = C_HEX;
            break;

          case 'o':
            scan_fn = (scan_function*)&scan_octal;
            cclass = C_OCTAL;
            break;
          default: buffer_putsflush(fd_err->w, "ERROR: expecting x|b|o\n"); return NULL;
        }

        if((classes & (C_UPPER | C_LOWER))) {
          digit = source_next(&c) > 0;
          n = 0;
        }
      }

    } while(digit && n < FMT_LONG);

    x[n] = '\0';

    scan_fn(x, &num);

    node = tree_newnode(A_NUM);
    node->narithnum.num = num;

    return node;
  } else if(parse_isalpha(c) || c == '_' || c == '$') {

    if(c == '$')
      source_skip();

    if(parse_param(p))
      return NULL;

    node = p->node;
    p->node = p->tree = NULL;

    if(node->id == N_ARGPARAM)
      node->nargparam.flag |= S_ARITH;

    return node;
  }

  return NULL;
}
