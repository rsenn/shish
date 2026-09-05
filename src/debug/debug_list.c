#include "../../lib/uint64.h"
#define DEBUG_NOCOLOR 1
#include "../debug.h"
#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../expand.h"
#include "../fd.h"
#include "../tree.h"

/* the parser emits an empty N_ARGSTR chunk around a substitution inside
 * quotes (e.g. "$x") purely to carry quoting state -- a sibling already
 * shows that same bit, so drop the chunk here rather than in the parse
 * tree itself, where expand still needs it (an all-empty quoted word
 * must still expand to ""). */
static int
debug_is_empty_placeholder(union node* node) {
  return node->id == N_ARGSTR && node->nargstr.stra.len == 0 &&
         !(node->nargstr.flag & ~S_TABLE);
}

/* debug a list/tree
 * ----------------------------------------------------------------------- */
void
debug_list(union node* n, int depth) {
  union node* node;
  int first = 1;

  debug_begin(0, depth >= 0 ? depth + 1 : depth);
  // debug_indent(1);
  // debug_s(DEBUG_BEGIN);
  // debug_newline(depth);

  for(node = n; node; node = node->next) {
    if(debug_is_empty_placeholder(node) && !(node == n && node->next == NULL))
      continue;

    if(!first) {
      debug_c(',');
      debug_newline(depth >= 0 ? depth + 1 : depth);
    }

    debug_node(node, depth >= -1 ? depth + 2 : depth);
    first = 0;
  }

  debug_end(depth);
}
#endif
