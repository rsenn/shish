#include "../bre.h"

int
bre_atom_matches(const char* pat, size_t alen, int c) {
  if(pat[0] == '\\') {
    if(pat[1] >= '1' && pat[1] <= '9')
      return 1;
    return pat[1] == c;
  }
  if(pat[0] == '.')
    return 1;
  if(pat[0] == '[' && alen > 1)
    return bre_bracket_matches(pat, alen, c);
  return pat[0] == c;
}