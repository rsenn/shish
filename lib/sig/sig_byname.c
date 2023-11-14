#include "../sig_internal.h"
#include "../str.h"

int
sig_byname(const char* name) {
  const sigtable_t* p;

  if(!str_case_diffn(name, "SIG", 3))
    name += 3;

  for(p = sigtable; p->name; p++)
    if(!str_case_diff(p->name, name))
      return p->number;
  return -1;
}
