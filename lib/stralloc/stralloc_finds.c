#include "../str.h"
#include "../stralloc.h"
/*
  return index to first occurance of data,
  otherwise return sa->len
*/
size_t
stralloc_finds(const stralloc* sa, const char* what) {
  return stralloc_findb(sa, what, str_len(what));
}
