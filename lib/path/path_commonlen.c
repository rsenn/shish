#include "../path_internal.h"
#include "../byte.h"
#include <assert.h>

/* Computes the matching prefix offsets for base and target paths.
 * Returns a SizePair containing the consumed lengths (bi, ti). */
size_t
path_commonlen(const char* base, const char* target) {
  size_t i = path_issep(*base) && path_issep(*target);

  for(;;) {
    size_t bn = path_component(base + i);
    size_t tn = path_component(target + i);

    if(!bn || !tn || bn != tn || str_diffn(base + i, target + i, bn))
      break;

    i += bn;

    if(!path_issep(base[i]) || !path_issep(target[i]))
      break;

    i++;
  }

  if(path_issep(base[i]) && path_issep(target[i]))
    i++;

  return i;
}