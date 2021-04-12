#include "../sh.h"

size_t
sh_fmtflags(char* dest, const union shopt* opts) {
  size_t i = 0;
  if(opts->unset)
    dest[i++] = 'u';
  if(opts->no_clobber)
    dest[i++] = 'C';
  if(opts->debug)
    dest[i++] = 'x';
  if(opts->exit_on_error)
    dest[i++] = 'e';
  if(opts->no_clobber)
    dest[i++] = 'C';
  return i;
}
