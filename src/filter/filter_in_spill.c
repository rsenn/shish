#include "../filter.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"

/* appends n bytes to the spill buffer; 0 on out of memory */
int
filter_in_spill(struct filter_in* in, const char* p, size_t n) {
  if(in->spill_len + n + 1 > in->spill_cap) {
    size_t cap = in->spill_cap ? in->spill_cap : 256;
    char* q;

    while(cap < in->spill_len + n + 1)
      cap *= 2;

    if(!(q = alloc_re(in->spill, cap)))
      return 0;

    in->spill = q;
    in->spill_cap = cap;
  }

  byte_copy(in->spill + in->spill_len, n, p);
  in->spill_len += n;
  return 1;
}
