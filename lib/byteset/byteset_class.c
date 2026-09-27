#include "../byteset.h"
#include "../byte.h"
#include <ctype.h>

int
byteset_class(unsigned char* set, const char* name, size_t len) {
#define CLASS(s, fn) \
  if(len == sizeof(s) - 1 && byte_diff(name, len, s) == 0) { \
    unsigned c; \
    for(c = 0; c < 256; c++) \
      if(fn((int)c)) \
        byteset_add(set, c); \
    return 1; \
  }
  CLASS("upper", isupper)
  CLASS("lower", islower)
  CLASS("alpha", isalpha)
  CLASS("digit", isdigit)
  CLASS("alnum", isalnum)
  CLASS("punct", ispunct)
  CLASS("graph", isgraph)
  CLASS("print", isprint)
  CLASS("cntrl", iscntrl)
  CLASS("blank", isblank)
  CLASS("space", isspace)
  CLASS("xdigit", isxdigit)
#undef CLASS
  return 0;
}
