#include "../source.h"
#include "../debug.h"
#include "../prompt.h"

struct source* source = 0;

/* gets more data from buffer (at least n + 1 chars)
 * doesn't advance buffer pointer, use input_skipcn() for that
 * ----------------------------------------------------------------------- */
int
source_peekn(char* c, unsigned n) {
  buffer* b = source->b;
  int ret = buffer_LEN(b);
  unsigned lookahead = n;

  for(;;) {
    char *x, *y;
    unsigned i, j;
    /* no data available, try to get some */
    if((unsigned)ret <= lookahead) {
      if((ret = buffer_prefetch(b, lookahead + 1)) <= 0)
        return ret;
#ifdef DEBUG
      debug_ulong("source_peekn", ret, 0);
      debug_nl_fl();
#endif
    }
    x = buffer_PEEK(b);
    y = buffer_END(b);
    j = y - x;

    for(i = 0; i < lookahead + 1; i++) {

      if(x[i] == '\\') {
        if(i + 1 >= j) {
          if((ret = buffer_prefetch(b, i + 1)) <= 0)
            return ret;
          y = buffer_END(b);
          j = y - x;
        }
        if(x[i + 1] == '\n') {
          if(i == 0) {
            b->p += 2;
            source_newline();
          } else {
            n += 2;
          }
        }
      }
    }
    break;
  }
  /* got data, peek the char */
  if(c) {

    *c = b->x[b->p + n];
  }

  return ret;
}

int
source_peeknc(unsigned pos) {
  char c;
  if(source_peekn(&c, pos) <= 0)
    return -1;
  return (unsigned int)(unsigned char)c;
}
