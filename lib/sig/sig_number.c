#include "../sig_internal.h"

#include <strings.h>

int
sig_number(const char* name) {
  sigtable_t const* p = sigtable;

  for(; p->name; p++)
    if(!strcasecmp(name, p->name))
      break;

  return p->number;
}
