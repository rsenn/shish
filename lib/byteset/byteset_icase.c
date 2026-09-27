#include "../byteset.h"

void
byteset_icase(unsigned char* set) {
  unsigned c;

  for(c = 'a'; c <= 'z'; c++)
    if(byteset_has(set, c))
      byteset_add(set, c - 'a' + 'A');

  for(c = 'A'; c <= 'Z'; c++)
    if(byteset_has(set, c))
      byteset_add(set, c - 'A' + 'a');
}
