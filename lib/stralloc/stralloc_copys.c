#include "../str.h"
#include "../stralloc.h"

/* copies a \0 - terminated string from buf into sa, without the \0. */
int
stralloc_copys(stralloc* sa, const char* buf) {
  return stralloc_copyb(sa, buf, str_len(buf));
}
