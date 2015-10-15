unsigned long var_len(const char *v) {
  register const char *s = v;
  for(;;) {
    if(!*s || *s == '=') break; ++s;
    if(!*s || *s == '=') break; ++s;
    if(!*s || *s == '=') break; ++s;
    if(!*s || *s == '=') break; ++s;
  }
  return s - v;
}
