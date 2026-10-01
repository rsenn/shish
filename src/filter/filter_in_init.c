#include "../filter.h"
#include "../../lib/buffer.h"
#include "../../lib/byte.h"

/* initialize a filter_in tracking structure with error arguments, file
 * operands list, and fallback upstream input buffer.
 * ----------------------------------------------------------------------- */
void
filter_in_init(struct filter_in* in, char** errargv, char** files, buffer* upstream) {
  byte_zero(in, sizeof(*in));
  in->errargv = errargv;
  in->files = files;
  in->upstream = upstream;
}
