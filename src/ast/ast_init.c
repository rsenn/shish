#include "../ast.h"

void
ast_init(struct ast* a, buffer* b, int indent) {
  json_init(&a->j, b, indent);
  a->loc = 1;
  a->range = 0;
  a->no_position = 0;
}
