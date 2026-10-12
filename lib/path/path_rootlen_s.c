#include "../path.h"
#include "../byte.h"

int
path_rootlen_s(const char* p) {
  return path_rootlen(p, byte_chr(p, 3, '\0'));
}
