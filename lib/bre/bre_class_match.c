#include "../bre.h"
#include "../byte.h"
#include <ctype.h>

int
bre_class_match(const char* name, size_t namelen, int c) {
#define CLASS(s, fn) \
  if(namelen == sizeof(s) - 1 && !byte_diff(name, namelen, s)) \
  return !!fn((unsigned char)c)
  CLASS("upper", isupper);
  CLASS("lower", islower);
  CLASS("alpha", isalpha);
  CLASS("digit", isdigit);
  CLASS("alnum", isalnum);
  CLASS("punct", ispunct);
  CLASS("graph", isgraph);
  CLASS("print", isprint);
  CLASS("cntrl", iscntrl);
  CLASS("blank", isblank);
  CLASS("space", isspace);
  CLASS("xdigit", isxdigit);
#undef CLASS
  return 0;
}
