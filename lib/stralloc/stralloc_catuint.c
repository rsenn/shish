#include "../stralloc.h"

size_t
stralloc_catuint(stralloc* sa, unsigned int ui) {
  return stralloc_catulong0(sa, ui, 0);
}
