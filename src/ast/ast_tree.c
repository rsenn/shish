#include "../ast.h"

/* a whole script as one JSON document
 * ----------------------------------------------------------------------- */
void
ast_tree(struct ast* a, union node* list) {
  ast_list(a, list);
  buffer_putc(a->j.b, '\n');
  buffer_flush(a->j.b);
}
