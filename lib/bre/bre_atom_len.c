#include "../bre.h"

size_t
bre_atom_len(const char* pat) {
  if(pat[0] == '\\')
    return pat[1] ? 2 : 1;
  if(pat[0] == '[') {
    const char* p = pat + 1;
    if(*p == '^')
      p++;
    if(*p == ']')
      p++;
    while(*p && *p != ']') {
      if(p[0] == '[' && (p[1] == ':' || p[1] == '.' || p[1] == '=')) {
        char delim = p[1];
        const char* q = p + 2;
        while(*q && !(q[0] == delim && q[1] == ']'))
          q++;
        if(q[0] == delim && q[1] == ']') {
          p = q + 2;
          continue;
        }
      }
      p++;
    }
    if(*p == ']')
      return (size_t)(p + 1 - pat);
  }
  return 1;
}
