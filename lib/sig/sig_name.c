#include "../sig_internal.h"

const char*
sig_name(int sig) {
  sigtable_t const* p = sigtable;
  for(; p->number; p++)
    if(sig == p->number)
      break;
  return p->number ? p->name : "???";
}
