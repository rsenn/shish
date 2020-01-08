#include "fmt.h"
#include "parse.h"
#include "scan.h"
#include "source.h"
#include "tree.h"
#include "uint64.h"
#include "expand.h"

/* parse arithmetic value
 * ----------------------------------------------------------------------- */
union node*
parse_arith_value(struct parser* p) {
  char c;
  union node* node = NULL;

  if(source_peek(&c) <= 0)
    return NULL;

  if(c == '(')
    return parse_arith_paren(p);

  if(parse_isdigit(c)) {
    char x[FMT_LONG + 1];
    unsigned int n = 0;
    int64 num;

    do {
      x[n++] = c;

      if(source_next(&c) <= 0)
        break;

    } while(parse_isdigit(c) && n < FMT_LONG);

    x[n] = '\0';

    scan_longlong(x, &num);

    node = tree_newnode(N_ARITH_NUM);
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
