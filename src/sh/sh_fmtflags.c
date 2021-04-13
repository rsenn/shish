#include "../sh.h"

size_t
sh_fmtflags(char* dest, const struct shopt* opts) {
  size_t i = 0;
  if(opts->errexit)
    dest[i++] = 'e';
  if(opts->noglob)
    dest[i++] = 'f';
  if(opts->hashall)
    dest[i++] = 'h';
  if(opts->monitor)
    dest[i++] = 'm';
  if(opts->unset)
    dest[i++] = 'u';
  if(opts->xtrace)
    dest[i++] = 'x';
  if(opts->braceexpand)
    dest[i++] = 'B';
  if(opts->noclobber)
    dest[i++] = 'C';
  if(opts->histexpand)
    dest[i++] = 'H';

  return i;
}
