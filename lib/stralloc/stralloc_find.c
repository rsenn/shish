#include "../stralloc.h"
/*
  return index to first occurance of data,
  otherwise return sa->len
*/
size_t
stralloc_find(const stralloc* sa, const stralloc* what) {
  return stralloc_findb(sa, what->s, what->len);
}
