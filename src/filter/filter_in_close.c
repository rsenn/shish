#include "../filter.h"
#include "../../lib/buffer.h"
#include "../../lib/alloc.h"

/* close the current file buffer if it is a dedicated file operand,
 * leaving the external upstream buffer intact.
 * ----------------------------------------------------------------------- */
void
filter_in_close(struct filter_in* in) {
  if(in->cur && in->cur != in->upstream)
    buffer_close(in->cur);

  in->cur = NULL;

  if(!in->spilling) {
    alloc_free(in->spill);
    in->spill = NULL;
    in->spill_len = in->spill_cap = 0;
  }
}
