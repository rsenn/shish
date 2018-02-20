unsigned long str_copyn(char *out,const char *in, unsigned long n) {
  register unsigned long i;
  register const char* t=in;
  for(i=0;;) {
    if(i==n) break; if(!(out[i]=*t)) break; ++i; ++t;
    if(i==n) break; if(!(out[i]=*t)) break; ++i; ++t;
    if(i==n) break; if(!(out[i]=*t)) break; ++i; ++t;
    if(i==n) break; if(!(out[i]=*t)) break; ++i; ++t;
  }
  out[i]='\0';
  return i;
}
