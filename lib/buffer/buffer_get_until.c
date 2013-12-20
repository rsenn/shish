#include "byte.h"
#include "buffer.h"
#include "scan.h"

int buffer_get_until(buffer* b,char* x,unsigned long int len,const char* charset,unsigned long int setlen) {
  int blen;

  for (blen=0;blen<len;) {
    register int r;
    if ((r=buffer_getc(b,x))<0) return r;
    if (r==0) { *x=0; break; }
    blen++;
    if (byte_chr(charset,setlen,*x++)<setlen) break;
  }
  return blen;
}
