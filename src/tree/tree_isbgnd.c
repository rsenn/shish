#include "../tree.h"

/* does this node end a statement that "&" can follow? (the bgnd bit sits at the same place in all of them)
 * ----------------------------------------------------------------------- */
int
tree_isbgnd(const union node* node) {
  switch(node->id) {
    case N_SIMPLECMD:
    case N_PIPELINE:
    case N_AND:
    case N_OR:
    case N_NOT:
    case N_TIME:
    case N_SUBSHELL:
    case N_BRACEGROUP:
    case N_FOR:
    case N_CASE:
    case N_IF:
    case N_WHILE:
    case N_UNTIL: return node->ncmd.bgnd;
    default: return 0;
  }
}
