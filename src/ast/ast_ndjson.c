#include "../ast.h"
#include "../tree.h"

/* one top-level command per line, each a complete one-line document
 * ----------------------------------------------------------------------- */
void
ast_ndjson(struct ast* a, union node* list) {
  int indent = a->j.indent;
  union node* node;

  a->j.indent = 0;

  for(node = list; node; node = node->next) {
    ast_node(a, node);
    buffer_putc(a->j.b, '\n');
    buffer_flush(a->j.b);
  }

  a->j.indent = indent;
}
