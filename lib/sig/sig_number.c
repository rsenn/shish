#include "../sig.h"

#include <strings.h>
#include "sig-internal.h"

int
sig_number(char const* name) {
  sigtable_t const* p = sigtable;
  for(; p->name; p++)
    if(!strcasecmp(name, p->name))
      break;
  return p->number;
}
