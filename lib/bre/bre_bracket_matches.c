#include "../bre.h"

int
bre_bracket_matches(const char* pat, size_t alen, int c) {
  const char* p = pat + 1;
  const char* end = pat + alen - 1;
  int neg = 0, matched = 0;
  if(*p == '^') {
    neg = 1;
    p++;
  }
  while(p < end) {
    if(p[0] == '[' && p + 1 < end && (p[1] == ':' || p[1] == '.' || p[1] == '=')) {
      char delim = p[1];
      const char* q = p + 2;
      while(q < end && !(q[0] == delim && q[1] == ']'))
        q++;
      if(q < end && q[0] == delim && q[1] == ']') {
        if(delim == ':') {
          if(bre_class_match(p + 2, (size_t)(q - (p + 2)), c))
            matched = 1;
        } else if((size_t)(q - (p + 2)) == 1 && p[2] == c) {
          matched = 1;
        }
        p = q + 2;
        continue;
      }
    }
    if(p + 2 < end && p[1] == '-' && p[2] != ']') {
      if((unsigned char)c >= (unsigned char)p[0] && (unsigned char)c <= (unsigned char)p[2])
        matched = 1;
      p += 3;
      continue;
    }
    if(*p == c)
      matched = 1;
    p++;
  }
  return neg ? !matched : matched;
}