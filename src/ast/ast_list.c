#include "../ast.h"
#include "../expand.h"
#include "../tree.h"

/* the parser emits an empty N_ARGSTR chunk around a substitution inside quotes ("$x") only to
 * carry quoting state; a sibling already shows that bit, so it is dropped here -- expand still
 * needs it in the tree (an all-empty quoted word must expand to "")
 * ----------------------------------------------------------------------- */
static int
ast_placeholder(union node* node) {
  return node->id == N_ARGSTR && node->nargstr.len == 0 && !(node->nargstr.flag & ~S_TABLE);
}

/* a node list as a JSON array; a lone placeholder is kept so the list is not empty
 * ----------------------------------------------------------------------- */
void
ast_list(struct ast* a, union node* list) {
  union node* node;

  json_open(&a->j, '[');

  for(node = list; node; node = node->next)
    if(!ast_placeholder(node) || (node == list && !node->next))
      ast_node(a, node);

  json_close(&a->j, ']');
}
